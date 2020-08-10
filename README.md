# HB-RC-4-Dis-TH
## 4fach Wandtaster mit Display und Temperatursensor

Als Vorlage/Idee diente das [AVM FRITZ!DECT 440](https://avm.de/produkte/fritzdect/fritzdect-440/).

### Prototyp:

An den 4 Ecken befinden sich Taster, um über 4 verschiedenen Tasterkanäle Geräte zu steuern.<br/>
Unten mittig ist der Mode-Taster:
- langes Drücken = Anlernen oder (ganz lange) RESET
- kurzes Drücken = Anzeige der Tasterbeschriftung

In Ruhe werden auf dem Display die Temperatur (und wahlweise Luftfeuchtigkeit) sowie der Batteriezustand angezeigt.
Wird der Mode-Taster kurz gedrückt, erscheint für die in den Geräteeinstellungen festgelegte "Display Timeout" Zeit die Taster-Beschriftung.

Als Beschriftung lassen sich
- eine Überschrift (optional)
- 1 bis 2 Zeilen Beschriftungstext

hinterlegen

Nur Temperaturanzeige:<br>
<img src="Images/IMG_1488.jpg" width=500 /><br><br>
Temperatur- und Luftfeuchtigkeit:<br>
<img src="Images/IMG_1491.jpg" width=500 /><br><br>
Tasterbeschriftungen:<br>
<img src="Images/IMG_1492.jpg" width=500 />

### CCU:

**Status und Bedienung -> Geräte**<br/>
<img src="Images/CCU_Bedienung.png" width=500 />
<br/>
**Einstellungen -> Geräte**<br/>
<img src="Images/CCU_Einstellungen.png" width=500 />

### Hardware:

- ATmega **1284P**
- GoodDisplay ePaper 1,54" [**GDEW0154M09**](https://de.aliexpress.com/item/4000993819257.html)
- SHT31 Sensor (kann aber auch was anderes sein... SHT10, BME280 etc.pp.)
