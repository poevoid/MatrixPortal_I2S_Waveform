# MatrixPortal_I2S_Waveform
Two Arduino sketches that work in tandem on different microcontrollers to display a waveform on a 64x32 RGB matrix. Uses an ESP32 to interface with an I2S mic (INMP441) and output the audio stream through the board's 8-bit DAC on IO26, which goes to the Adafruit Matrix Portal M4's A2 pin. The Matrix Portal does some processing of the signal and displays the waveform on the 64x32 RGB matrix. Waveform is displayed as a scrolling/moving rainbow line.  

Tie all the 5V lines together *after* uploading code to each board, otherwise you'll have issues getting the upload to be successful. If you want, it can make things easier if you add simple on/off switches between each microcontroller and the main 5V input line. This way you can put the switches to "off" position and upload code normally to the respective boards. 

![20260109_142948](https://github.com/user-attachments/assets/f2350edd-b289-4494-b345-b6a2169a872b)
