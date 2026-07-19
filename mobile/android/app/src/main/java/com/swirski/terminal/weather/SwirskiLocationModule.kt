package com.swirski.terminal.weather

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.location.Location
import android.location.LocationManager
import androidx.core.content.ContextCompat
import com.facebook.react.bridge.Arguments
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod

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

      if (location == null) {
        promise.reject("LOCATION_UNAVAILABLE", "No recent phone location is available")
        return
      }

      val coordinates = Arguments.createMap()
      coordinates.putDouble("latitude", location.latitude)
      coordinates.putDouble("longitude", location.longitude)
      promise.resolve(coordinates)
    } catch (error: Exception) {
      promise.reject("LOCATION_FAILED", "Could not read phone location", error)
    }
  }
}
