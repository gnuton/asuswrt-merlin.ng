# A new GNUton's ASUS Merlin build is available for your DSL-AC68U!
<p align="center">
  <img src="logo.jpg">
</p>  
Since Asuswrt-Merlin is mostly a variant of the original Asuswrt, it means that there is no special procedure to flash it.  Just flash it the same way you would flash any regular Asus firmware.

## Procedure:
* Click the download button below in this page
* Download the .trx file
* Upload the trx to http://router.asus.com/Advanced_FirmwareUpgrade_Content.asp
* If anything goes wrong please check the notes below

## Troubleshooting
* If the router rejects the firmware due to low memory please reboot the router before flashing and disconnect the USB key if any.
* If some features are not working please reset to factory the router before raising an issue.
* It is very hard to brick an Asus router.  If something goes wrong during flashing, you can put your router in Recovery mode by powering it on while you keep Reset pressed.  After your release it, the power LED will either blink or stay off (depending on the model).  At that point, you can either access it through http://192.168.1.1 (make sure you first give your PC a static IP within the same range, e.g. 192.168.1.100), or through the Firmware Recovery Tool provided on Asus's support CD.  You will then be able to flash a working firmware.
* It is _not_ recommended to restore settings saved under a different firmware version.  It _might_ work, but there is no guarantee.

<p align="center">
  <a href="https://github.com/gnuton/asuswrt-merlin.ng/releases/latest"><img src="download-button.png"></a>
</p>
