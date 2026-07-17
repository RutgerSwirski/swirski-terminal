package com.swirski.terminal.media

import android.content.ComponentName
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.os.Handler
import android.os.Looper
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.modules.core.DeviceEventManagerModule
import com.swirski.terminal.notifications.SwirskiNotificationListenerService
import org.json.JSONObject

class SwirskiMediaModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  private val mainHandler = Handler(Looper.getMainLooper())
  private val controllerCallbacks =
    mutableMapOf<MediaController, MediaController.Callback>()
  private var lastEmittedMusicState: MusicState? = null
  private var pendingMusicStateEmit: Runnable? = null

  private val listenerComponent =
    ComponentName(
      reactContext,
      SwirskiNotificationListenerService::class.java,
    )

  private val activeSessionsChangedListener =
    MediaSessionManager.OnActiveSessionsChangedListener { controllers ->
      updateControllerCallbacks(controllers.orEmpty())
      emitCurrentMusicState()
    }

  init {
    startListeningForMediaChanges()
  }

  override fun getName(): String = "SwirskiMedia"

  @ReactMethod
  fun addListener(eventName: String) {
  }

  @ReactMethod
  fun removeListeners(count: Int) {
  }

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
        createMusicStateMessageJson(
          messageId = messageId,
          musicState = musicState,
        ),
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

    return mediaSessionManager
      .getActiveSessions(listenerComponent)
      .mapNotNull { controller -> musicStateFromController(controller) }
      .sortedByDescending { state -> state.isPlaying }
      .firstOrNull()
  }

  private fun startListeningForMediaChanges() {
    try {
      val mediaSessionManager =
        reactApplicationContext.getSystemService(MediaSessionManager::class.java)
          ?: return

      mediaSessionManager.addOnActiveSessionsChangedListener(
        activeSessionsChangedListener,
        listenerComponent,
        mainHandler,
      )

      updateControllerCallbacks(
        mediaSessionManager.getActiveSessions(listenerComponent),
      )
    } catch (error: SecurityException) {
      // Notification listener access may not be enabled yet.
    }
  }

  private fun updateControllerCallbacks(
    controllers: List<MediaController>,
  ) {
    val activeControllers = controllers.toSet()

    controllerCallbacks
      .filterKeys { controller -> controller !in activeControllers }
      .forEach { (controller, callback) ->
        controller.unregisterCallback(callback)
        controllerCallbacks.remove(controller)
      }

    controllers.forEach { controller ->
      if (controllerCallbacks.containsKey(controller)) {
        return@forEach
      }

      val callback = object : MediaController.Callback() {
        override fun onMetadataChanged(metadata: MediaMetadata?) {
          emitCurrentMusicState()
        }

        override fun onPlaybackStateChanged(state: PlaybackState?) {
          emitCurrentMusicState()
        }
      }

      controller.registerCallback(callback, mainHandler)
      controllerCallbacks[controller] = callback
    }
  }

  private fun emitCurrentMusicState() {
    try {
      val musicState = getCurrentMusicState() ?: return

      if (musicState == lastEmittedMusicState) {
        return
      }

      scheduleMusicStateEmit(musicState)
    } catch (error: Exception) {
      // Media updates are best-effort; connect-time sync can still work.
    }
  }

  private fun scheduleMusicStateEmit(musicState: MusicState) {
    pendingMusicStateEmit?.let { pendingEmit ->
      mainHandler.removeCallbacks(pendingEmit)
    }

    val emitMusicState = Runnable {
      pendingMusicStateEmit = null

      if (musicState == lastEmittedMusicState) {
        return@Runnable
      }

      lastEmittedMusicState = musicState

      reactApplicationContext
        .getJSModule(DeviceEventManagerModule.RCTDeviceEventEmitter::class.java)
        .emit(
          MUSIC_STATE_CHANGED_EVENT,
          createMusicStateMessageJson(
            messageId = "mobile-music-${System.currentTimeMillis()}",
            musicState = musicState,
          ),
        )
    }

    pendingMusicStateEmit = emitMusicState
    mainHandler.postDelayed(
      emitMusicState,
      MUSIC_STATE_EMIT_DEBOUNCE_MS,
    )
  }

  private fun createMusicStateMessageJson(
    messageId: String,
    musicState: MusicState,
  ): String {
    return JSONObject()
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
      .toString()
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

  companion object {
    const val MUSIC_STATE_CHANGED_EVENT = "SwirskiMusicStateChanged"
    private const val MUSIC_STATE_EMIT_DEBOUNCE_MS = 750L
  }
}
