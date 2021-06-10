# Room-Light-Changer
Smart device capable of controlling light status of my room via internet 
---
---
## How Does It Works?
---
It uses a webpage as front end interface to interact with a real time database stored in firebase to modify variables that are interpreted by an ESP32 to control light status, due to low internet capabilities in my room, a second microcontroller "Arduino Nano" reads constantly the inputs of ultrasonic sensors to keep track of the amount of people inside the room and the absolute temperature and humidity, those data is sent to ESP32 through UART serial protocol and uploaded to firebase.

---
# Interface:

![](https://github.com/PaoloReyes254/Room-Light-Changer/blob/main/Images/Interface.PNG?raw=true)