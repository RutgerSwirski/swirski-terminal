package com.swirski.terminal.weather

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.location.Geocoder
import android.location.Location
import android.location.LocationManager
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import java.util.Locale

class SwirskiLocationModule(
  reactContext: ReactApplicationContext,
) : ReactContextBaseJavaModule(reactContext) {
  override fun getName(): String = "SwirskiLocation"

  @ReactMethod
  fun getLastKnownCoordinates(promise: Promise) {
    val hasPermission =
      ContextCompat.checkSelfPermission(
        reactApplicationContext,
        Manifest.permission.ACCESS_COARSE_LOCATION,
      ) == PackageManager.PERMISSION_GRANTED ||
        ContextCompat.checkSelfPermission(
          reactApplicationContext,
          Manifest.permission.ACCESS_FINE_LOCATION,
        ) == PackageManager.PERMISSION_GRANTED

    if (!hasPermission) {
      promise.reject("LOCATION_PERMISSION_REQUIRED", "Location permission is required")
      return
    }

    try {
      val locationManager =
        reactApplicationContext.getSystemService(Context.LOCATION_SERVICE) as LocationManager

      val location =
        locationManager
          .getProviders(true)
          .mapNotNull { provider -> locationManager.getLastKnownLocation(provider) }
          .maxByOrNull { item: Location -> item.time }

      if (location != null) {
        val locationName = resolveLocationName(location)
        saveLocation(location, locationName)
        promise.resolve(
          createCoordinates(
            location.latitude,
            location.longitude,
            locationName,
          ),
        )
        return
      }

      val savedLocation = getSavedLocation()

      if (savedLocation == null) {
        promise.reject("LOCATION_UNAVAILABLE", "No recent phone location is available")
        return
      }

      promise.resolve(savedLocation)
    } catch (error: Exception) {
      promise.reject("LOCATION_FAILED", "Could not read phone location", error)
    }
  }

  private fun createCoordinates(
    latitude: Double,
    longitude: Double,
    locationName: String?,
  ) = Arguments.createMap().apply {
    putDouble("latitude", latitude)
    putDouble("longitude", longitude)
    putString("locationName", locationName)
  }

  private fun saveLocation(location: Location, locationName: String?) {
    reactApplicationContext
      .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
      .edit()
      .putLong(LATITUDE_KEY, location.latitude.toBits())
      .putLong(LONGITUDE_KEY, location.longitude.toBits())
      .putString(LOCATION_NAME_KEY, locationName)
      .apply()
  }

  private fun getSavedLocation() =
    reactApplicationContext
      .getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE)
      .takeIf {
        it.contains(LATITUDE_KEY) && it.contains(LONGITUDE_KEY)
      }
      ?.let {
        createCoordinates(
          Double.fromBits(it.getLong(LATITUDE_KEY, 0)),
          Double.fromBits(it.getLong(LONGITUDE_KEY, 0)),
          it.getString(LOCATION_NAME_KEY, null),
        )
      }

  @Suppress("DEPRECATION")
  private fun resolveLocationName(location: Location): String? {
    if (!Geocoder.isPresent()) {
      return null
    }

    return try {
      val address =
        Geocoder(reactApplicationContext, Locale.getDefault())
          .getFromLocation(location.latitude, location.longitude, 1)
          ?.firstOrNull()
          ?: return null
      val area =
        address.locality
          ?: address.subAdminArea
          ?: address.adminArea
      val parts =
        listOfNotNull(area, address.countryCode)
          .map(String::trim)
          .filter(String::isNotEmpty)
          .distinct()

      parts.joinToString(", ").takeIf(String::isNotEmpty)
    } catch (_: Exception) {
      null
    }
  }

  companion object {
    private const val PREFERENCES = "swirski_weather"
    private const val LATITUDE_KEY = "latitude"
    private const val LONGITUDE_KEY = "longitude"
    private const val LOCATION_NAME_KEY = "location_name"
  }
}
