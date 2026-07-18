package com.swirski.terminal.background

import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.os.Build
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class SwirskiBackgroundModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = "SwirskiBackground"

  @ReactMethod
  fun requestEnableBluetooth(promise: Promise) {
    val activity = reactApplicationContext.currentActivity

    if (activity == null) {
      promise.reject("NO_ACTIVITY", "Could not open the Bluetooth prompt")
      return
    }

    activity.startActivity(
      Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE),
    )
    promise.resolve(null)
  }

  @ReactMethod
  fun start(deviceId: String, promise: Promise) {
    try {
      val intent = Intent(
        reactApplicationContext,
        SwirskiConnectionService::class.java,
      ).putExtra(SwirskiConnectionService.EXTRA_DEVICE_ID, deviceId)

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        reactApplicationContext.startForegroundService(intent)
      } else {
        reactApplicationContext.startService(intent)
      }

      promise.resolve(null)
    } catch (error: Exception) {
      promise.reject("BACKGROUND_START_FAILED", "Could not start connection service", error)
    }
  }

  @ReactMethod
  fun stop(promise: Promise) {
    reactApplicationContext.stopService(
      Intent(reactApplicationContext, SwirskiConnectionService::class.java),
    )
    SwirskiConnectionService.clearSavedDeviceId(reactApplicationContext)
    promise.resolve(null)
  }

  @ReactMethod
  fun getSavedDeviceId(promise: Promise) {
    promise.resolve(
      SwirskiConnectionService.getSavedDeviceId(reactApplicationContext),
    )
  }
}
