package com.swirski.terminal.updates

import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.swirski.terminal.BuildConfig

class SwirskiUpdatesModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = "SwirskiUpdates"

  @ReactMethod
  fun getCurrentBuild(promise: Promise) {
    val build = Arguments.createMap().apply {
      putInt("versionCode", BuildConfig.VERSION_CODE)
      putString("versionName", BuildConfig.VERSION_NAME)
    }

    promise.resolve(build)
  }
}
