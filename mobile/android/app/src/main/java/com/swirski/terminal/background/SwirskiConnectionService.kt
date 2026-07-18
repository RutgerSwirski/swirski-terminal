package com.swirski.terminal.background

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import com.swirski.terminal.MainActivity
import com.swirski.terminal.R

class SwirskiConnectionService : Service() {
  override fun onCreate() {
    super.onCreate()

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      val channel = NotificationChannel(
        CHANNEL_ID,
        "Terminal connection",
        NotificationManager.IMPORTANCE_LOW,
      )

      getSystemService(NotificationManager::class.java)
        .createNotificationChannel(channel)
    }
  }

  override fun onStartCommand(
    intent: Intent?,
    flags: Int,
    startId: Int,
  ): Int {
    val deviceId = intent?.getStringExtra(EXTRA_DEVICE_ID)

    if (!deviceId.isNullOrBlank()) {
      saveDeviceId(this, deviceId)
    }

    val openAppIntent = PendingIntent.getActivity(
      this,
      0,
      Intent(this, MainActivity::class.java),
      PendingIntent.FLAG_IMMUTABLE or PendingIntent.FLAG_UPDATE_CURRENT,
    )

    val notificationBuilder = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      Notification.Builder(this, CHANNEL_ID)
    } else {
      Notification.Builder(this)
    }

    val notification = notificationBuilder
      .setContentTitle("Swirski Terminal connected")
      .setContentText("Keeping the wearable connection active")
      .setSmallIcon(R.mipmap.ic_launcher)
      .setContentIntent(openAppIntent)
      .setOngoing(true)
      .build()

    startForeground(NOTIFICATION_ID, notification)
    return START_STICKY
  }

  override fun onBind(intent: Intent?): IBinder? = null

  companion object {
    private const val CHANNEL_ID = "swirski_terminal_connection"
    private const val NOTIFICATION_ID = 1001
    private const val PREFERENCES = "swirski_connection"
    private const val DEVICE_ID_KEY = "device_id"
    const val EXTRA_DEVICE_ID = "device_id"

    fun getSavedDeviceId(context: Context): String? {
      return context
        .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
        .getString(DEVICE_ID_KEY, null)
    }

    fun saveDeviceId(context: Context, deviceId: String) {
      context
        .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
        .edit()
        .putString(DEVICE_ID_KEY, deviceId)
        .apply()
    }

    fun clearSavedDeviceId(context: Context) {
      context
        .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
        .edit()
        .remove(DEVICE_ID_KEY)
        .apply()
    }
  }
}
