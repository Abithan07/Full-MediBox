## Full-MediBox
Medibox is a device that assists users in managing their medication schedules effectively. This project involves the design and implementation of a Medibox with the following features.

Features
1. Time Zone and Alarms
    * Set time zone by entering the offset from UTC.
    * Set up to 3 alarms.
    * Disable all alarms.
    * Fetch the current time from an NTP server over Wi-Fi and display it on an OLED screen.
    * Trigger alarms with visual and auditory indicators, stoppable using a push button.

2. Environmental Monitoring
    * Monitor temperature and humidity levels.
    * Provide warnings when temperature exceeds 26°C to 32°C and/or humidity exceeds 60% to 80%.

3. Light Intensity Monitoring
    * Measure light intensity using two LDRs placed on either side of the Medibox.
    * Display the highest light intensity on the Node-RED dashboard using a gauge and plot.
    * Indicate which LDR (left or right) detects the maximum intensity.

4. Light Control with Shaded Sliding Window
    * A servo motor adjusts a shaded sliding window based on light intensity.
    * The motor angle is controlled by the equation:

```bash
    θ = min{θoffset × D + (180 − θoffset) × I × γ, 180}
```

  * θ: Motor angle
  * θoffset: Minimum angle (default: 30 degrees)
  * I: Maximum light intensity (0 to 1)
  * γ: Controlling factor (default: 0.75)
  * D: 0.5 if right LDR gives max intensity, 1.5 if left LDR gives max intensity

        
5. Node-RED Dashboard Controls

    * Sliders to adjust the minimum angle (0 to 120 degrees) and the controlling factor (0 to 1).
   Different medicines may have different requirements for the minimum angle and the controlling factor used to adjust the position of the shaded sliding window.
    * Dropdown menu for medicine selection with predefined settings for the minimum angle and controlling factor, or a custom option for manual adjustments.


### Technologies Used
  * Node-RED for dashboard interface
  * Wi-Fi for NTP time synchronization
  * OLED display for time and alarms
  * Sensors for temperature, humidity, and light intensity monitoring
  * Servo motor for adjusting the shaded sliding window


Wokwi Project link: https://wokwi.com/projects/397626177508504577

![Screenshot 2024-07-27 035259](https://github.com/user-attachments/assets/1cf37984-3bec-4dca-8c49-14b8a528a0f8)


### Customizations
1. Button Configuration
    * Replaced the traditional 4-button configuration with a 2-button configuration, minimalizing the look of the Medibox without sacrificing functionalities.

2. Node-RED Flow
    * Implemented a new Node-RED flow to enhance the dashboard interface and control functionalities.

## This is the flow of my node-red implementation

![Screenshot 2024-07-27 022752](https://github.com/user-attachments/assets/c42db0a6-86b3-490a-8acb-cc30ec6ba905)


Reference: [chathura-de-silva/Smart Medibox](https://github.com/chathura-de-silva/Smart-Medibox.git)
