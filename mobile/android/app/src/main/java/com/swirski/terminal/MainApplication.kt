package com.swirski.terminal

import android.app.Application
import com.facebook.react.PackageList
import com.facebook.react.ReactApplication
import com.facebook.react.ReactHost
import com.facebook.react.ReactNativeApplicationEntryPoint.loadReactNative
import com.facebook.react.defaults.DefaultReactHost.getDefaultReactHost
import com.swirski.terminal.background.SwirskiBackgroundPackage
import com.swirski.terminal.media.SwirskiMediaPackage
import com.swirski.terminal.notifications.SwirskiNotificationsPackage

class MainApplication : Application(), ReactApplication {

  override val reactHost: ReactHost by lazy {
    getDefaultReactHost(
      context = applicationContext,
      packageList =
        PackageList(this).packages.apply {
          // Packages that cannot be autolinked yet can be added manually here, for example:
          add(SwirskiBackgroundPackage())
          add(SwirskiNotificationsPackage())
          add(SwirskiMediaPackage())
        },
    )
  }

  override fun onCreate() {
    super.onCreate()
    loadReactNative(this)
  }
}
