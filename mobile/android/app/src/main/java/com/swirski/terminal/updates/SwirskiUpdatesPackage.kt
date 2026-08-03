package com.swirski.terminal.updates

import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.uimanager.ViewManager

class SwirskiUpdatesPackage : ReactPackage {
  @Deprecated(
    "ReactPackage.createNativeModules is deprecated upstream, but this is the simplest local bridge.",
  )
  override fun createNativeModules(
    reactContext: ReactApplicationContext,
  ): List<NativeModule> = listOf(SwirskiUpdatesModule(reactContext))

  override fun createViewManagers(
    reactContext: ReactApplicationContext,
  ): List<ViewManager<*, *>> = emptyList()
}
