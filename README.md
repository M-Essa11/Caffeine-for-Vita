# Caffeine for Vita

Caffeine for Vita adds a sleep-prevention toggle to the PS Vita Quick Menu.
When enabled, it keeps the display awake and prevents automatic suspend. Manual
standby remains available.

The toggle starts disabled after every reboot. Its state and blue cup indicator
remain synchronized when the Quick Menu is closed and reopened.

The current stable release is **v1.0.1**.

![Caffeine cup icon](assets/caffeine.png)

## Requirements

- A homebrew-enabled PS Vita or PS TV with taiHEN
- VitaSDK
- QuickMenuReborn 2.5

The VPK installer includes QuickMenuReborn 2.5 and installs it automatically.

## VPK installation

1. Install `CaffeineForVita.vpk` with VitaShell.
2. Open **Caffeine Installer**.
3. Press **X** to install.
4. Reboot when prompted.

The installer copies the plugin to `ur0:QuickMenuReborn`, adds the
QuickMenuReborn loader under `*main`, and backs up `ur0:tai/config.txt`. It also
comments out an active NoSleep entry because running both plugins would prevent
Caffeine from restoring normal sleep behavior.

After rebooting, the VPK file and installer bubble may be deleted. The Quick
Menu plugin remains installed.

## Standalone plugin installation

Use this method if QuickMenuReborn 2.5 is already installed:

1. Download `caffeine.suprx` and `caffeine.png` from the release assets.
2. Copy both files to `ur0:QuickMenuReborn/` with VitaShell.
3. Disable any active NoSleep plugin entry in `ur0:tai/config.txt` so normal
   sleep behavior can return when Caffeine is off.
4. Reboot the Vita.

QuickMenuReborn must already be loaded under `*main`. The standalone files do
not install or configure QuickMenuReborn automatically.

## Uninstallation

Open **Caffeine Installer** and press **Square**. The installer removes Caffeine
and the bundled QuickMenuReborn files, removes its loader entry, and restores a
NoSleep line previously disabled by the installer. Reboot, then delete the
installer bubble.

For a standalone installation, delete `ur0:QuickMenuReborn/caffeine.suprx` and
`ur0:QuickMenuReborn/caffeine.png`, then reboot.

## Building

Install VitaSDK and the taiHEN package, then run from WSL or Linux:

```sh
vdpm install taihen
./tools/build.sh
```

The resulting installer is written to `out/CaffeineForVita.vpk`. The build
script uses directories under the Linux home folder because VitaSDK packaging
tools do not reliably handle spaces in source paths.

## Credits

- [QuickMenuReborn](https://github.com/Ibrahim778/QuickMenuReborn) by Ibrahim778
- [NoSleep](https://github.com/NamelessGhoul0/nosleep) as a reference for Vita
  power-tick behavior

## License

Caffeine for Vita is released under the [MIT License](LICENSE).
