package com.swirski.terminal.notifications

import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.uimanager.ViewManager

class SwirskiNotificationsPackage : ReactPackage {
  @Deprecated(
    "ReactPackage.createNativeModules is deprecated upstream, but this is the simplest local bridge.",
  )
  override fun createNativeModules(
    reactContext: ReactApplicationContext,
  ): List<NativeModule> {
    return listOf(
      SwirskiNotificationsModule(reactContext),
    )
  }

  override fun createViewManagers(
    reactContext: ReactApplicationContext,
  ): List<ViewManager<*, *>> {
    return emptyList()
  }
}
