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

# Implementing Siri voice commands to control light status

I found that the app shortcuts in an Iphone can execute python codes, so I will use this IOS capability to execute the modifications of firebase variables through shortcuts called by Siri.

Since all this code will be written in an iphone I will document this process in this README file.

The first thing to do is to create a python code that can connect and create instances of firebase and the only replicate the functions my ON and OFF buttons are doing.
