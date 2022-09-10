# Custom Game Controller

This is a mod for Cyberpunk 2077 that enables raw joystick/game controller input and configuration. Multiple devices (like two joysticks) can be used at the same time. For examples of configurations, [check out the redscript folder](https://github.com/jackhumbert/ctd_helper/tree/main/src/redscript/ctd_helper). Below is a some of the documentation for `ICustomGameController`:

```swift
﻿public abstract native class ICustomGameController extends IScriptable {
  public native let pid: Uint16;
  public native let vid: Uint16;
  public native let id: Int32;
  public native let buttons: array<Bool>;
  // This is the enum used by `switches` interally:
  //   Center    = 0
  //   Up        = 1
  //   UpRight   = 2
  //   Right     = 3
  //   DownRight = 4
  //   Down      = 5
  //   DownLeft  = 6
  //   Left      = 7
  //   UpLeft    = 8
  public native let switches: array<Uint32>;
  public native let axes: array<Float>;
 
  public native let buttonKeys: array<EInputKey>;
  public native let axisKeys: array<EInputKey>;
  public native let axisInversions: array<Bool>;
  public native let axisCenters: array<Float>;
  public native let axisDeadzones: array<Float>;

  // Maps a controller button to a key
  //   button: 0-indexed
  //   key: https://nativedb.red4ext.com/EInputKey
  public native func SetButton(button: Int32, key: EInputKey);

  // Maps a controller axis to a key
  //   axis: 0-indexed
  //   key: https://nativedb.red4ext.com/EInputKey
  //   inverted: whether or not the axis should be inverted
  //   center: value between 0.0-1.0 that acts as the natural position of the axis
  //   deadzone: value between 0.0-1.0 that acts as a threshold for movement
  public native func SetAxis(axis: Int32, key: EInputKey, inverted: Bool, center: Float, deadzone: Float);

  // Called after the controller is created but not intialized (no values are read yet)
  // Use this to call SetButton & SetAxis
  public abstract func OnSetup() -> Void;

  // Called after new values are read, but before they're assigned
  public abstract func OnUpdate() -> Void;

  // Called when axis values are read from the controller, after center & deadzone correction
  //   index: 0-indexed axis index
  //   value: axis value to be compared & assigned if different
  public abstract func GetAxisValue(index: Uint32, value: Float) -> Float;
}
```

## Installation

[Get the latest release here](https://github.com/jackhumbert/ctd_helper/releases) - `packed-v*.zip` in the release contains all of the requirements listed below at their most up-to-date versions (at the time of release). Simply extract it and copy the contents in your game's installation folder.

If you want to install the mod outside of a release (not recommended), the `build/` folder in the repo contains all of the mod-specific files that you can drag into your game's installation folder.

## Configuration

Configuration is done through redscript classes. Some examples are provided.

## Requirements

* [RED4ext](https://github.com/WopsS/RED4ext)
* [Redscript](https://github.com/jac3km4/redscript)
* [Input Loader](https://github.com/jackhumbert/cyberpunk2077-input-loader)

## Bugs

If you come across something that doesn't work quite right, or interferes with another mod, [search for or create an issue!](https://github.com/jackhumbert/ctd_helper/issues) I have a lot of things on a private TODO list still, but can start to move things to Github issues.

Special thanks to @psiberx for [Codeware Lib](https://github.com/psiberx/cp2077-codeware/), [InkPlayground Demo](https://github.com/psiberx/cp2077-playground), and Redscript & CET examples on Discord, @WopsS for [RED4ext](https://github.com/WopsS/RED4ext), @jac3km4 for [Redscript toolkit](https://github.com/jac3km4/redscript), @yamashi for [CET](https://github.com/yamashi/CyberEngineTweaks), @rfuzzo & team (especially @seberoth!) for [WolvenKit](https://github.com/WolvenKit/WolvenKit), and all of them for being helpful on Discord.
