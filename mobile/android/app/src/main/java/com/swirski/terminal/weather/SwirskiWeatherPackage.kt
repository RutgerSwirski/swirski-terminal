package com.swirski.terminal.weather

import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.uimanager.ViewManager

class SwirskiWeatherPackage : ReactPackage {
  @Deprecated(
    "ReactPackage.createNativeModules is deprecated upstream, but this is the simplest local bridge.",
  )
  override fun createNativeModules(
    reactContext: ReactApplicationContext,
  ): List<NativeModule> = listOf(SwirskiLocationModule(reactContext))

  override fun createViewManagers(
    reactContext: ReactApplicationContext,
  ): List<ViewManager<*, *>> = emptyList()
}
