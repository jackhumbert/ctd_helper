# Crash-To-Desktop Helper

[![](https://byob.yarr.is/jackhumbert/ctd_helper/cp_version)](https://github.com/jackhumbert/ctd_helper/actions/workflows/build.yaml)

This will output a useful log to `red4ext/logs/ctd_helper.log` upon a crash in Cyberpunk 2077.

# Installation

This requires RED4ext - install the single .dll to `red4ext/plugins/ctd_helper/ctd_helper.dll`.

# How to read the logs

Tabs illustrate the call hierarchy. `>` is the most recent call, and the numbers now count down to the latest (crash happens during `DebugBreak`):

```Log
Thread hash: 7869061899589541243
  Func:   OnGameAttach()
  Class:  FlightComponent
  7 Func:   GetVehicle()
    Class:  FlightComponent
    6 Func:   GetEntity()
      Class:  entIComponent
  5 Func:   GetGame()
    Class:  gameObject
  4 Func:   GetFloat()
    Class:  FlightSettings
  3 Func:   GetFloat()
    Class:  FlightSettings
  2 Func:   FlightModeHoverFly::Create(FlightComponent)
  Func:   Initialize(FlightComponent)
  Class:  FlightModeHoverFly
  1 Func:   Initialize(FlightComponent)
    Class:  FlightMode
    > Func:   DebugBreak()
      Class:  FlightSettings
```

Source files & (appx) line numbers are also shown for CDPR classes (modded classes won't show this yet):

```Log
Thread hash: 9473072060749266088 LAST EXECUTED
  Func:   OnRequestComponents()
  Class:  sampleBullet
  7 Func:   OnRequestComponents()
    Class:  BaseProjectile
    Source: samples\sampleBullet.ws:15
    6 Func:   RequestComponent()
      Class:  entEntityRequestComponentsInterface
      Source: cyberpunk\projectiles\baseProjectile.script:35
    5 Func:   RequestComponent()
      Class:  entEntityRequestComponentsInterface
      Source: cyberpunk\projectiles\baseProjectile.script:35
    4 Func:   RequestComponent()
      Class:  entEntityRequestComponentsInterface
      Source: samples\sampleBullet.ws:15
    3 Func:   OnRequestComponents()
      Class:  BaseProjectile
      Source: samples\sampleBullet.ws:15
      2 Func:   RequestComponent()
        Class:  entEntityRequestComponentsInterface
        Source: cyberpunk\projectiles\baseProjectile.script:35
      1 Func:   RequestComponent()
        Class:  entEntityRequestComponentsInterface
        Source: cyberpunk\projectiles\baseProjectile.script:35
      > Func:   RequestComponent()
        Class:  entEntityRequestComponentsInterface
        Source: samples\sampleBullet.ws:15
```

`LAST EXECUTED` is added to the last executed thread as well, but this doesn't always mean this is where the crash occurred.
