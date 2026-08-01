package com.swirski.terminal.notifications

import android.app.Notification
import android.os.Handler
import android.os.Looper
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import android.util.Log
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.modules.core.DeviceEventManagerModule
import org.json.JSONArray
import org.json.JSONObject
import java.util.concurrent.ConcurrentHashMap

class SwirskiNotificationListenerService : NotificationListenerService() {
    private val emitHandler = Handler(Looper.getMainLooper())
    private val pendingNotificationEmits =
        ConcurrentHashMap<String, Runnable>()

    override fun onListenerConnected() {
        refreshSnapshot()
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val incoming = createTerminalNotification(sbn)
        val previous = notifications[incoming.id]

        if (!incoming.hasVisibleText()) {
            val removed = notifications.remove(incoming.id)

            cancelPendingNotificationEmit(incoming.id)

            if (removed != null) {
                emitNotificationRemoved(incoming.id)
            }

            return
        }

        val notification =
            if (previous != null) {
                incoming.copy(postedAt = previous.postedAt)
            } else {
                incoming
            }

        val isNew = previous == null
        val contentChanged = previous != notification

        notifications[notification.id] = notification

        if (!contentChanged) {
            return
        }

        if (isNew) {
            emitNotificationUpserted(
                notification = notification,
                alert = true,
            )
        } else {
            scheduleNotificationUpserted(
                notification = notification,
                alert = false,
            )
        }

        Log.d(
            TAG,
            if (isNew) {
                "Inserted notification: ${notification.id}"
            } else {
                "Updated notification: ${notification.id}"
            },
        )
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        val notificationId =
            createNotificationId(sbn)

        cancelPendingNotificationEmit(notificationId)

        val removed =
            notifications.remove(notificationId)

        if (removed != null) {
            emitNotificationRemoved(notificationId)
        }

        Log.d(
            TAG,
            "Removed notification: $notificationId",
        )
    }

    private fun refreshSnapshot() {
        notifications.clear()

        activeNotifications.orEmpty().forEach { sbn ->
            val notification = createTerminalNotification(sbn)

            if (notification.hasVisibleText()) {
                notifications[notification.id] = notification
            }
        }

        Log.d(TAG, "Refreshed notification snapshot: ${notifications.size}")
    }

    private fun createTerminalNotification(
        sbn: StatusBarNotification,
    ): TerminalNotification {
        val androidNotification =
            sbn.notification

        val extras =
            androidNotification.extras

        return TerminalNotification(
            id = createNotificationId(sbn),
            packageName = sbn.packageName,
            appName = getAppName(sbn.packageName),
            title =
                extras
                    .getCharSequence(Notification.EXTRA_TITLE)
                    ?.toString()
                    .orEmpty(),
            body =
                extras
                    .getCharSequence(Notification.EXTRA_BIG_TEXT)
                    ?.toString()
                    ?: extras
                        .getCharSequence(Notification.EXTRA_TEXT)
                        ?.toString()
                        .orEmpty(),
            postedAt = sbn.postTime,
            ongoing = sbn.isOngoing,
            category = androidNotification.category,
        )
    }

    private fun scheduleNotificationUpserted(
        notification: TerminalNotification,
        alert: Boolean,
    ) {
        cancelPendingNotificationEmit(notification.id)

        val emitNotification = Runnable {
            pendingNotificationEmits.remove(notification.id)

            emitNotificationUpserted(
                notification = notification,
                alert = alert,
            )
        }

        pendingNotificationEmits[notification.id] = emitNotification

        emitHandler.postDelayed(
            emitNotification,
            NOTIFICATION_EMIT_DEBOUNCE_MS,
        )
    }

    private fun cancelPendingNotificationEmit(notificationId: String) {
        val pendingEmit =
            pendingNotificationEmits.remove(notificationId)
                ?: return

        emitHandler.removeCallbacks(pendingEmit)
    }

    private fun getAppName(packageName: String): String {
        return try {
            val appInfo = packageManager.getApplicationInfo(packageName, 0)

            val appName = packageManager.getApplicationLabel(appInfo).toString()

            if (appName.isBlank() || appName == packageName || isGenericAppName(appName, packageName)) {
                readableNameFromPackageName(packageName)
            } else {
                appName
            }
        } catch (_: Exception) {
            readableNameFromPackageName(packageName)
        }
    }

    private fun readableNameFromPackageName(packageName: String): String {
        val packageParts = packageName
            .split(".")
            .filter { part -> part.isNotBlank() }

        val lastSegment = packageParts
            .asReversed()
            .firstOrNull { part -> !isGenericPackageSegment(part) }
            ?: packageParts.lastOrNull()
            ?: packageName

        if (lastSegment.isBlank()) {
            return packageName
        }

        return lastSegment
            .replace("-", " ")
            .replace("_", " ")
            .split(" ")
            .filter { word -> word.isNotBlank() }
            .joinToString(" ") { word ->
                word.replaceFirstChar { char ->
                    if (char.isLowerCase()) char.titlecase() else char.toString()
                }
            }
    }

    private fun isGenericAppName(appName: String, packageName: String): Boolean {
        return appName.equals("Android", ignoreCase = true) && packageName != "android"
    }

    private fun isGenericPackageSegment(segment: String): Boolean {
        return segment.equals("android", ignoreCase = true)
    }

    companion object {
        private const val TAG = "SwirskiNotifications"
        const val NOTIFICATION_UPSERTED_EVENT = "SwirskiNotificationUpserted"
        const val NOTIFICATION_REMOVED_EVENT = "SwirskiNotificationRemoved"
        private const val NOTIFICATION_EMIT_DEBOUNCE_MS = 750L
        private const val MAX_NOTIFICATIONS = 40

        private val notifications =
            ConcurrentHashMap<String, TerminalNotification>()

        private var reactContext: ReactApplicationContext? = null

        fun setReactContext(context: ReactApplicationContext) {
            reactContext = context
        }

        fun getSnapshot(): List<TerminalNotification> {
            return notifications.values.sortedByDescending { notification ->
                notification.postedAt
            }.take(MAX_NOTIFICATIONS)
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

        private fun emitNotificationUpserted(
            notification: TerminalNotification,
            alert: Boolean,
        ) {
            val context = reactContext ?: return

            val messageJson =
                createNotificationUpsertedMessageJson(
                    messageId =
                        "mobile-notification-${System.currentTimeMillis()}",
                    notification = notification,
                    alert = alert,
                )

            try {
                context
                    .getJSModule(
                        DeviceEventManagerModule
                            .RCTDeviceEventEmitter::class.java,
                    )
                    .emit(
                        NOTIFICATION_UPSERTED_EVENT,
                        messageJson,
                    )
            } catch (error: Exception) {
                Log.d(
                    TAG,
                    "Could not emit notification upsert",
                    error,
                )
            }
        }

        private fun createNotificationUpsertedMessageJson(
            messageId: String,
            notification: TerminalNotification,
            alert: Boolean,
        ): String {
            val payload =
                JSONObject()
                    .put("alert", alert)
                    .put(
                        "notification",
                        notification.toJson(),
                    )

            return JSONObject()
                .put("version", 1)
                .put("type", "notification.upserted")
                .put("id", messageId)
                .put("payload", payload)
                .toString()
        }

        private fun emitNotificationRemoved(notificationId: String) {
            val context = reactContext ?: return

            val messageJson = JSONObject()
                .put("version", 1)
                .put("type", "notification.removed")
                .put("id", "mobile-notification-removed-${System.currentTimeMillis()}")
                .put(
                    "payload",
                    JSONObject().put("id", notificationId),
                )
                .toString()

            try {
                context
                    .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
                    .emit(NOTIFICATION_REMOVED_EVENT, messageJson)
            } catch (error: Exception) {
                Log.d(TAG, "Could not emit notification removal", error)
            }
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
        val ongoing: Boolean,
        val category: String?,
    ) {
        fun hasVisibleText(): Boolean {
            return title.isNotBlank() ||
                    body.isNotBlank()
        }

        fun toJson(): JSONObject {
            return JSONObject()
                .put("id", id)
                .put("packageName", packageName)
                .put("appName", appName)
                .put("title", title)
                .put("body", body)
                .put("postedAt", postedAt)
                .put("ongoing", ongoing)
                .put(
                    "category",
                    category ?: JSONObject.NULL,
                )
        }
    }
}
