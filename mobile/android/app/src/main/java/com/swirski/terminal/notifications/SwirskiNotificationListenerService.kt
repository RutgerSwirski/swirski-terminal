package com.swirski.terminal.notifications

import android.app.Notification
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import java.util.concurrent.ConcurrentHashMap

class SwirskiNotificationListenerService : NotificationListenerService() {
  override fun onListenerConnected() {
    refreshSnapshot()
  }

  override fun onNotificationPosted(sbn: StatusBarNotification) {
    val notification = createTerminalNotification(sbn)

    notifications[notification.id] = notification

    Log.d(TAG, "Stored notification: ${notification.id}")
  }

  override fun onNotificationRemoved(sbn: StatusBarNotification) {
    notifications.remove(createNotificationId(sbn))

    Log.d(TAG, "Removed notification: ${sbn.key}")
  }

  private fun refreshSnapshot() {
    notifications.clear()

    activeNotifications.orEmpty().forEach { sbn ->
      val notification = createTerminalNotification(sbn)

      notifications[notification.id] = notification
    }

    Log.d(TAG, "Refreshed notification snapshot: ${notifications.size}")
  }

  private fun createTerminalNotification(
    sbn: StatusBarNotification,
  ): TerminalNotification {
    val extras = sbn.notification.extras

    return TerminalNotification(
      id = createNotificationId(sbn),
      packageName = sbn.packageName,
      appName = getAppName(sbn.packageName),
      title = extras.getCharSequence(Notification.EXTRA_TITLE)?.toString().orEmpty(),
      body =
        extras.getCharSequence(Notification.EXTRA_BIG_TEXT)?.toString()
          ?: extras.getCharSequence(Notification.EXTRA_TEXT)?.toString().orEmpty(),
      postedAt = sbn.postTime,
    )
  }

  private fun getAppName(packageName: String): String {
    return try {
      val appInfo = packageManager.getApplicationInfo(packageName, 0)

      packageManager.getApplicationLabel(appInfo).toString()
    } catch (_: Exception) {
      packageName
    }
  }

  companion object {
    private const val TAG = "SwirskiNotifications"

    private val notifications =
      ConcurrentHashMap<String, TerminalNotification>()

    fun getSnapshot(): List<TerminalNotification> {
      return notifications.values.sortedByDescending { notification ->
        notification.postedAt
      }
    }

    fun createSnapshotMessageJson(messageId: String): String {
      val payload = JSONObject()
        .put(
          "notifications",
          JSONArray(
            getSnapshot().map { notification ->
              notification.toJson()
            },
          ),
        )

      return JSONObject()
        .put("version", 1)
        .put("type", "notifications.snapshot")
        .put("id", messageId)
        .put("payload", payload)
        .toString()
    }

    private fun createNotificationId(sbn: StatusBarNotification): String {
      return sbn.key
    }
  }

  data class TerminalNotification(
    val id: String,
    val packageName: String,
    val appName: String,
    val title: String,
    val body: String,
    val postedAt: Long,
  ) {
    fun toJson(): JSONObject {
      return JSONObject()
        .put("id", id)
        .put("packageName", packageName)
        .put("appName", appName)
        .put("title", title)
        .put("body", body)
        .put("postedAt", postedAt)
    }
  }
}
