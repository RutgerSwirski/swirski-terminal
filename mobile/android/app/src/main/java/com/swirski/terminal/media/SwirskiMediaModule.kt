package com.swirski.terminal.media

import android.content.ComponentName
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.swirski.terminal.notifications.SwirskiNotificationListenerService
import org.json.JSONObject

class SwirskiMediaModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = "SwirskiMedia"

  @ReactMethod
  fun getCurrentMusicStateMessageJson(
    messageId: String,
    promise: Promise,
  ) {
    try {
      val musicState = getCurrentMusicState()

      if (musicState == null) {
        promise.resolve(null)
        return
      }

      promise.resolve(
        JSONObject()
          .put("version", 1)
          .put("type", "music.state")
          .put("id", messageId)
          .put(
            "payload",
            JSONObject()
              .put("appName", musicState.appName)
              .put("title", musicState.title)
              .put("artist", musicState.artist)
              .put("isPlaying", musicState.isPlaying),
          )
          .toString(),
      )
    } catch (error: SecurityException) {
      promise.reject(
        "MEDIA_ACCESS_DENIED",
        "Notification listener access is required to read media sessions",
        error,
      )
    } catch (error: Exception) {
      promise.reject(
        "MEDIA_STATE_FAILED",
        "Could not read current music state",
        error,
      )
    }
  }

  private fun getCurrentMusicState(): MusicState? {
    val mediaSessionManager =
      reactApplicationContext.getSystemService(MediaSessionManager::class.java)
        ?: return null

    val listenerComponent =
      ComponentName(
        reactApplicationContext,
        SwirskiNotificationListenerService::class.java,
      )

    return mediaSessionManager
      .getActiveSessions(listenerComponent)
      .mapNotNull { controller -> musicStateFromController(controller) }
      .sortedByDescending { state -> state.isPlaying }
      .firstOrNull()
  }

  private fun musicStateFromController(
    controller: MediaController,
  ): MusicState? {
    val metadata = controller.metadata ?: return null

    val title =
      metadata.getString(MediaMetadata.METADATA_KEY_TITLE)
        ?: metadata.getString(MediaMetadata.METADATA_KEY_DISPLAY_TITLE)
        ?: return null

    if (title.isBlank()) {
      return null
    }

    val artist =
      metadata.getString(MediaMetadata.METADATA_KEY_ARTIST)
        ?: metadata.getString(MediaMetadata.METADATA_KEY_ALBUM_ARTIST)
        ?: metadata.getString(MediaMetadata.METADATA_KEY_DISPLAY_SUBTITLE)
        ?: ""

    return MusicState(
      appName = getAppName(controller.packageName),
      title = title,
      artist = artist,
      isPlaying = controller.playbackState?.state == PlaybackState.STATE_PLAYING,
    )
  }

  private fun getAppName(packageName: String): String {
    return try {
      val appInfo = reactApplicationContext.packageManager
        .getApplicationInfo(packageName, 0)

      val appName = reactApplicationContext.packageManager
        .getApplicationLabel(appInfo)
        .toString()

      if (appName.isBlank() || appName == packageName) {
        readableNameFromPackageName(packageName)
      } else {
        appName
      }
    } catch (_: Exception) {
      readableNameFromPackageName(packageName)
    }
  }

  private fun readableNameFromPackageName(packageName: String): String {
    return packageName
      .split(".")
      .lastOrNull { part -> part.isNotBlank() && part != "android" }
      ?.replaceFirstChar { char ->
        if (char.isLowerCase()) char.titlecase() else char.toString()
      }
      ?: "Music"
  }

  private data class MusicState(
    val appName: String,
    val title: String,
    val artist: String,
    val isPlaying: Boolean,
  )
}
