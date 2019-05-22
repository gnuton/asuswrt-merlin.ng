# New firmware release available!
<p align="center">
  <img src="logo.jpg">
</p>  
Since Asuswrt-Merlin is mostly a variant of the original Asuswrt, it means that there is no special procedure to flash it.  Just flash it the same way you would flash any regular Asus firmware.

A few notes:
* It's recommended to reboot the router _before_ flashing it, to free up sufficient memory.  Otherwise, there's a chance the router might reject the firmware, due to running too low of free RAM.
* While it is generally not necessary to restore to factory defaults, it's not a bad idea, especially if there is a big jump in version number (e.g. from 112 to 178).  No need to do the 30/30/30 dance as required by DD-WRT - just do a plain Factory Default reset, or turn the device on while keeping the WPS button pressed.  (The procedure can be different from one model to another.)
* It is very hard to brick an Asus router.  If something goes wrong during flashing, you can put your router in Recovery mode by powering it on while you keep Reset pressed.  After your release it, the power LED will either blink or stay off (depending on the model).  At that point, you can either access it through http://192.168.1.1 (make sure you first give your PC a static IP within the same range, e.g. 192.168.1.100), or through the Firmware Recovery Tool provided on Asus's support CD.  You will then be able to flash a working firmware.
* If something looks weird, don't waste too much time: save your settings, reset to factory default, reconfigure the basics, and see if the issue is resolved.  If not, you can always restore your saved settings and do some more advanced troubleshooting.
* It is _not_ recommended to restore settings saved under a different firmware version.  It _might_ work, but there is no guarantee.

<p align="center">
  <a href="https://github.com/gnuton/asuswrt-merlin.ng/releases/latest"><img src="download-button.png"></a>
</p>
