
<br>

<div align="center">
  
  ## User_Setup Files

</div>

<b>These files allow you to easy define a user setup for the ttgo t-display in Arduino IDE. <br>
Before compiling a sketch in IDE you should define the board as a "LOLIN D32" and set the  partiton to "minimal with spiffs".</b>

---

# Directions for use

- If you do not have the TFT_eSPI library by Bodmer installed (it's included in the lib folder in my repo as well), go ahead a download it from <a href=https://github.com/Bodmer/TFT_eSPI>HERE</a> and manually install/add it to your library folder
  Or install it through Arduinos library manager.

- Make a full backup of your \Arduino\libraries\ folder and keep it somewhere safe to revert to if you run into any errors.

- Add these User_Setup files to your "C:\Users\YOURNAME\Documents\Arduino\libraries\TFT_eSPI-master" folder. (Your folder might be titled slightly different but look for TFT_eSPI folder.)
  
  <br>
  
  After you have them added to the folder, go into the "User_Setup_Select" file and make sure #include <User_Setup_TTGO_NoTouch.h> is uncommented. 
  It should be when you download it but just double check.



