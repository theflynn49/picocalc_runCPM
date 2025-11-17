# picocalc-runCPM
![runcpm_picocalc](https://github.com/user-attachments/assets/2458fa54-2655-4d5b-84d9-264f62ececc6)

This is a port of runCPM on the Picocalc using picocalc-text-framwork
This is a RP2040 only version, but **Randorandus** reports that it works on a RP2350W.

# Credits

- runCPM Copyright (c) 2017 Mockba the Borg  (https://github.com/MockbaTheBorg/RunCPM)
- picocalc-text-framework Copyright (c) 2025 Blair Leduc  (https://github.com/BlairLeduc/picocalc-text-starter)
- Largely inspired by the work of Guido Lehwalder (https://github.com/guidol70/RunCPM_RPi_Pico)
<br>
Thank you to these guys for the great packages.

# Installation

- copy the uf2 file you'll find in the release section to your picocalc, or the SD card if you use an uf2 loader.
- create an "A" directory on the root of the SD card, as described in the runCPM repo : [DISK_A](https://github.com/MockbaTheBorg/RunCPM/tree/master/DISK)
- create the other drives you'll find here : [CPM Toolset](https://obsolescence.wixsite.com/obsolescence/multicomp-fpga-cpm-demo-disk) "Download Tool Set"
- copy the file [TINST.DTA](support_files/TINST.DTA) over /E/3/TINST.DTA to get a working TP3 Editor 
<br>
To test TP3, enter these commands :
<br>

```
E:
USER 3
TURBO
```
<br>
WS does work, now that we have 80 columns.

# Updates

## v1.2

Thanx to **rugosi** we have now 80 columns.

## v1.1a

I made a boo-boo using Ctrl-S and Ctrl-A for XON/XOFF, since these keys are used by all editors. So XOFF is now KEY_F5 and XON is KEY_F4

## v1.1

- Pico speed faster (to 200Mhz)
- Supports BRK to some extent (returns to CP/M)
- Supports XON/XOFF (to stop long DIRs and such)
- Added BELL

## Bugs

Probably a lot, all to blame on me. Use this only for fun, no guarantee for anything. You've been warned.
I haven't done any extensive tests, but Turbo Pascal 3.0 seems to work, including the editor functions, and the Tinstaller TINST.COM so it should work pretty well to some extent.
<br>
<br>
Happy CPMing !
