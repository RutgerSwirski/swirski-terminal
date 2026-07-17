package com.swirski.terminal.notifications

import android.content.ComponentName
import android.content.Intent
import android.provider.Settings
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

class SwirskiNotificationsModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  init {
    SwirskiNotificationListenerService.setReactContext(reactContext)
  }

  override fun getName(): String = "SwirskiNotifications"

  @ReactMethod
  fun addListener(eventName: String) {
  }

  @ReactMethod
  fun removeListeners(count: Int) {
  }

  @ReactMethod
  fun isNotificationAccessEnabled(promise: Promise) {
    promise.resolve(isNotificationAccessEnabled())
  }

  @ReactMethod
  fun openNotificationAccessSettings(promise: Promise) {
    try {
      val intent = Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)

      reactApplicationContext.startActivity(intent)

      promise.resolve(null)
    } catch (error: Exception) {
      promise.reject(
        "OPEN_SETTINGS_FAILED",
        "Could not open notification access settings",
        error,
      )
    }
  }

  @ReactMethod
  fun createSnapshotMessageJson(
    messageId: String,
    promise: Promise,
  ) {
    try {
      promise.resolve(
        SwirskiNotificationListenerService.createSnapshotMessageJson(messageId),
      )
    } catch (error: Exception) {
      promise.reject(
        "SNAPSHOT_FAILED",
        "Could not create notification snapshot",
        error,
      )
    }
  }

  private fun isNotificationAccessEnabled(): Boolean {
    val enabledListeners =
      Settings.Secure.getString(
        reactApplicationContext.contentResolver,
        "enabled_notification_listeners",
      ) ?: return false

    val expectedComponent =
      ComponentName(
        reactApplicationContext,
        SwirskiNotificationListenerService::class.java,
      )

    return enabledListeners
      .split(":")
      .mapNotNull { listener ->
        ComponentName.unflattenFromString(listener)
      }
      .any { listener ->
        listener == expectedComponent
      }
  }
}
