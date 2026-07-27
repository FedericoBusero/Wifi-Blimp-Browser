import logging
from mpos import Activity
import lvgl as lv
from mpos import WifiService

import asyncio
import aiohttp
import network

# Fri3d badge 2024
from mpos.board.fri3d_2024 import adc_up_down, adc_left_right, btn_y, btn_b, btn_a

JOYSTICK_RECTANGLE_WIDTH = const(80)
JOYSTICK_RECTANGLE_HEIGHT = const(80)
JOYSTICK_CIRCLE_RADIUS = const(30)


class Main(Activity):

    refresh_joystick_timer = None
    refresh_wifi_timer = None
    refresh_button_timer = None
    wifi_label = None
    ws_task = None
    
    # Joystick waarden (-180 tot 180)
    joy_x = 0
    joy_y = 0

    def get_wifi_ssid(self):
        ssid = WifiService.get_current_ssid()
        if ssid:
            return f"Wi-Fi: {ssid}"
        return "No Wifi"
    
    def onCreate(self):
        print("onCreate RemoteControl")
        screen = lv.obj()
        self.status_label = lv.label(screen)
        self.status_label.set_text("Trying to connect")
        self.status_label.align(lv.ALIGN.TOP_LEFT, 30, 30)

        # SSID Label bovenaan het scherm
        self.wifi_label = lv.label(screen)
        self.wifi_label.set_text(self.get_wifi_ssid())
        self.wifi_label.align(lv.ALIGN.TOP_MID, 0, 10)
        
        # Create a black rectangle with a green border
        self.rect = lv.obj(screen)
        self.rect.set_size(JOYSTICK_RECTANGLE_WIDTH, JOYSTICK_RECTANGLE_HEIGHT)
        self.rect.set_style_radius(0, lv.PART.MAIN)
        self.rect.set_style_bg_color(lv.color_black(), lv.PART.MAIN)
        self.rect.set_style_border_color(lv.color_hex(0x00FF00), lv.PART.MAIN)
        self.rect.set_style_border_width(1, lv.PART.MAIN)
        self.rect.remove_flag(lv.obj.FLAG.SCROLLABLE)
        self.rect.align(lv.ALIGN.TOP_LEFT, 30, 150)

        self.circ_area = lv.obj(self.rect)
        self.circ_area.set_size(JOYSTICK_CIRCLE_RADIUS, JOYSTICK_CIRCLE_RADIUS)
        self.circ_area.set_style_radius(lv.RADIUS_CIRCLE, lv.PART.MAIN)
        self.circ_area.set_style_bg_color(lv.color_hex(0x00FF00), lv.PART.MAIN)
        self.circ_area.set_style_border_width(0, lv.PART.MAIN)
        
        self.slider1 = lv.slider(screen)
        self.slider1.set_range(-1000, 1000)
        self.slider1.set_value(0, False)
        self.slider1.align(lv.ALIGN.TOP_LEFT, 30, 80)
        self.slider1.add_event_cb(self.compensate_joystick_cb, lv.EVENT.KEY, None)
        self.slider1.add_event_cb(self.on_slider_change, lv.EVENT.VALUE_CHANGED, None)
        
        # Label om de slider waarde te tonen
        self.slider1_label = lv.label(screen)
        self.slider1_label.set_text("Waarde: 0")
        self.slider1_label.align(lv.ALIGN.TOP_LEFT, 30, 100)

        self.setContentView(screen)

    def onStart(self, screen):
        print("starting joystick refresh_timer")
        self.refresh_joystick_timer = lv.timer_create(self.refresh_joystick, 80, None)
        self.refresh_wifi_timer = lv.timer_create(self.refresh_wifi, 10000, None)
        self.refresh_button_timer = lv.timer_create(self.refresh_buttons, 80, None)
        
        # Silence the MPOS focus_direction logger while this screen is active (joystick events)
        logging.getLogger("mpos.ui.focus_direction").setLevel(logging.ERROR)
        
        print("start websocket program started")
        # Maak een achtergrondtaak aan op de reeds draaiende asyncio loop
        loop = asyncio.get_event_loop()
        self.ws_task = loop.create_task(self.main_websocket())

    def onStop(self, screen):
        if self.refresh_joystick_timer:
            print("stopping joystick refresh_timer")
            self.refresh_joystick_timer.delete()

        if self.refresh_wifi_timer:
            print("stopping wifi refresh_timer")
            self.refresh_wifi_timer.delete()

        if self.refresh_button_timer:
            print("stopping button refresh_timer")
            self.refresh_button_timer.delete()

        # Stop de websocket taak als deze nog draait
        if self.ws_task:
            print("stopping websocket task")
            self.ws_task.cancel()
            self.ws_task = None
            
        # Restore default logging level when leaving
        logging.getLogger("mpos.ui.focus_direction").setLevel(logging.WARNING)

    def refresh_buttons(self, timer):
        if btn_y.value() == 0:
            current_value = self.slider1.get_value()
            new_value = min(1000, current_value + 10)
            self.slider1.set_value(new_value, False)
            self.on_slider_change(None)
        if btn_b.value() == 0:
            current_value = self.slider1.get_value()
            new_value = max(-1000, current_value - 10)
            self.slider1.set_value(new_value, False)
            self.on_slider_change(None)
        if btn_a.value() == 0:
            self.slider1.set_value(0, False)
            self.on_slider_change(None)

    def refresh_joystick(self, timer):
        # Fri3d badge 2024
        raw_y = adc_up_down.read()
        raw_x = adc_left_right.read()
        
        # Map de analoge waarden naar schermcoördinaten voor het bolletje
        if self.circ_area:
            x_pos = lv.map(raw_x, 0, 4095, -int(JOYSTICK_RECTANGLE_WIDTH/2), int(JOYSTICK_RECTANGLE_WIDTH/2))
            y_pos = lv.map(raw_y, 4095, 0, -int(JOYSTICK_RECTANGLE_HEIGHT/2), int(JOYSTICK_RECTANGLE_HEIGHT/2))
            self.circ_area.set_pos(x_pos + int(JOYSTICK_CIRCLE_RADIUS/2), y_pos + int(JOYSTICK_CIRCLE_RADIUS/2))

        # Map de ADC waarden naar het bereik -180 tot 180 voor de WebSocket
        self.joy_x = lv.map(raw_x, 0, 4095, -180, 180)
        self.joy_y = lv.map(raw_y, 4095, 0, -180, 180)

    def refresh_wifi(self, timer):
        if self.wifi_label:
            self.wifi_label.set_text(self.get_wifi_ssid())
            
    def on_slider_change(self, event):
        if self.slider1_label:
            self.slider1_label.set_text(f"Waarde: {self.slider1.get_value()}")

    def compensate_joystick_cb(self, e):
        if e.get_code() == lv.EVENT.KEY:
            key = e.get_key()
            slider_obj = self.slider1
            current_val = slider_obj.get_value()
            
            # Als de joystick RIGHT/UP/LEFT/DOWN key event geeft, wordt de waarde aangepast +/- 1, dan
            # doen we net omgekeerde om waarde weer goed te krijgen
            if key in (lv.KEY.RIGHT, lv.KEY.UP):
                slider_obj.set_value(current_val - 1, None)
                self.on_slider_change(None)
            elif key in (lv.KEY.LEFT, lv.KEY.DOWN):
                slider_obj.set_value(current_val + 1, None)
                self.on_slider_change(None)

    async def send_ping_loop(self, ws):
        """Sends periodic ping data to the WebSocket server."""
        try:
            while True:
                msg = "0"
                print(f"--> Sending ping: {msg}")
                await ws.send_str(msg)
                await asyncio.sleep(1)  # Send every second
        except asyncio.CancelledError:
            pass

    async def send_joystick_loop(self, ws):
        """Sends live joystick coordinates to the WebSocket server."""
        try:
            while True:
                msg = f"1:{self.joy_x},{self.joy_y}"
                print(f"--> Sending joystick: {msg}")
                await ws.send_str(msg)
                await asyncio.sleep(0.4)  # Send every 400ms
        except asyncio.CancelledError:
            pass

    async def receive_loop(self, ws):
        """Listens for incoming messages from the server."""
        try:
            async for msg in ws:
                if msg.type == aiohttp.WSMsgType.TEXT:
                    print(f"<-- Received: {msg.data}")
                    if self.status_label:
                        self.status_label.set_text(msg.data)
                elif msg.type in (aiohttp.WSMsgType.CLOSED, aiohttp.WSMsgType.ERROR):
                    print("WebSocket connection closed or encountered an error.")
                    break
        except asyncio.CancelledError:
            pass
        except Exception as e:
            print("Error in receive loop:", e)

    async def main_websocket(self):
        SERVER_IP = "192.168.4.1"
        PORT = 82
        url = f"ws://{SERVER_IP}:{PORT}/"
        
        while True:
            ping_sender = None
            joy_sender = None
            receiver = None
            
            try:
                print(f"Connecting to {url}...")
                if self.status_label:
                    self.status_label.set_text("Connecting...")

                async with aiohttp.ClientSession() as session:
                    async with session.ws_connect(url) as ws:
                        print("Connected to 192.168.4.1:82!")
                        if self.status_label:
                            self.status_label.set_text("Connected")
                        
                        ping_sender = asyncio.create_task(self.send_ping_loop(ws))
                        joy_sender = asyncio.create_task(self.send_joystick_loop(ws))
                        receiver = asyncio.create_task(self.receive_loop(ws))
                        
                        # await asyncio.gather(ping_sender, joy_sender, receiver)
                        await asyncio.gather(receiver)

            except asyncio.CancelledError:
                print("WebSocket main task gracefully cancelled.")
                break  # Stop de loop definitief wanneer de activiteit stopt (onStop)

            except (OSError, aiohttp.ClientError, Exception) as e:
                print(f"WebSocket connection lost or failed: {e}")
                if self.status_label:
                    self.status_label.set_text("Reconnecting ...")

            finally:
                # Zorg dat de subtaken altijd worden gestopt voordat we opnieuw verbinden
                if ping_sender:
                    ping_sender.cancel()
                if joy_sender:
                    joy_sender.cancel()
                if receiver:
                    receiver.cancel()

            # Wacht 5 seconden alvorens opnieuw te proberen
            print("Retrying connection in 5 seconds...")
            await asyncio.sleep(5)
