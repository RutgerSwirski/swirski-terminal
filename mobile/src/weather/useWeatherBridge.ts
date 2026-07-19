import { useCallback, useEffect } from 'react';
import { NativeModules, PermissionsAndroid, Platform } from 'react-native';
import type { Device } from 'react-native-ble-plx';

import type { ConnectionStatus } from '../ble/useTerminalBle';
import {
  createWeatherSnapshotMessage,
  type WeatherSnapshot,
} from '../protocol/messages';

type Coordinates = {
  latitude: number;
  longitude: number;
};

type SwirskiLocationModule = {
  getLastKnownCoordinates(): Promise<Coordinates>;
};

type WeatherApiResponse = {
  current?: {
    temperature_2m?: number;
    weather_code?: number;
  };
  daily?: {
    time?: string[];
    weather_code?: number[];
    temperature_2m_min?: number[];
    temperature_2m_max?: number[];
  };
};

const SwirskiLocation = NativeModules.SwirskiLocation as
  | SwirskiLocationModule
  | undefined;

function conditionForCode(code: number): string {
  if (code === 0) return 'Clear';
  if (code <= 3) return 'Cloudy';
  if (code <= 48) return 'Fog';
  if (code <= 57) return 'Drizzle';
  if (code <= 67) return 'Rain';
  if (code <= 77) return 'Snow';
  if (code <= 82) return 'Showers';
  if (code <= 86) return 'Snow showers';
  return 'Thunderstorm';
}

async function requestLocationPermission(): Promise<boolean> {
  if (Platform.OS !== 'android') {
    return false;
  }

  const result = await PermissionsAndroid.request(
    PermissionsAndroid.PERMISSIONS.ACCESS_COARSE_LOCATION,
  );

  return result === PermissionsAndroid.RESULTS.GRANTED;
}

async function fetchWeather(): Promise<WeatherSnapshot> {
  if (!SwirskiLocation) {
    throw new Error('Native location module is not available');
  }

  if (!(await requestLocationPermission())) {
    throw new Error('Location permission was not granted');
  }

  const { latitude, longitude } =
    await SwirskiLocation.getLastKnownCoordinates();
  const url =
    'https://api.open-meteo.com/v1/forecast' +
    `?latitude=${latitude}&longitude=${longitude}` +
    '&current=temperature_2m,weather_code' +
    '&daily=weather_code,temperature_2m_max,temperature_2m_min' +
    '&timezone=auto&forecast_days=4';
  const response = await fetch(url);

  if (!response.ok) {
    throw new Error(`Weather request failed with status ${response.status}`);
  }

  const data = (await response.json()) as WeatherApiResponse;
  const current = data.current;
  const daily = data.daily;
  const dates = daily?.time;
  const weatherCodes = daily?.weather_code;
  const minimumTemperatures = daily?.temperature_2m_min;
  const maximumTemperatures = daily?.temperature_2m_max;

  if (
    typeof current?.temperature_2m !== 'number' ||
    typeof current.weather_code !== 'number' ||
    !dates ||
    !weatherCodes ||
    !minimumTemperatures ||
    !maximumTemperatures
  ) {
    throw new Error('Weather response is incomplete');
  }

  const forecastCount = Math.min(
    4,
    dates.length,
    weatherCodes.length,
    minimumTemperatures.length,
    maximumTemperatures.length,
  );

  if (forecastCount === 0) {
    throw new Error('Weather response has no forecast days');
  }

  const forecast = dates.slice(0, forecastCount).map((date, index) => ({
    day: new Date(`${date}T12:00:00`)
      .toLocaleDateString('en-US', {
        weekday: 'short',
      })
      .toUpperCase(),
    condition: conditionForCode(weatherCodes[index]),
    lowC: Math.round(minimumTemperatures[index]),
    highC: Math.round(maximumTemperatures[index]),
  }));

  return {
    location: 'Current location',
    updatedAtMs: Date.now(),
    current: {
      temperatureC: Math.round(current.temperature_2m),
      condition: conditionForCode(current.weather_code),
    },
    forecast,
  };
}

type UseWeatherBridgeArgs = {
  connectedDevice: Device | null;
  connectionStatus: ConnectionStatus;
  sendBleMessage(
    device: Device,
    message: Record<string, unknown>,
  ): Promise<void>;
};

export function useWeatherBridge({
  connectedDevice,
  connectionStatus,
  sendBleMessage,
}: UseWeatherBridgeArgs) {
  const sendCurrentWeather = useCallback(
    async (device: Device) => {
      const weather = await fetchWeather();

      const isConnected = await device.isConnected();

      if (!isConnected) {
        console.log('Skipping weather update: terminal disconnected');
        return;
      }

      const message = createWeatherSnapshotMessage(weather);

      console.log('Sending weather snapshot:', {
        characters: JSON.stringify(message).length,
        mtu: device.mtu,
        message,
      });

      await sendBleMessage(device, message);

      console.log('Weather snapshot sent');
    },
    [sendBleMessage],
  );

  useEffect(() => {
    if (!connectedDevice || connectionStatus !== 'ready') {
      return;
    }

    const interval = setInterval(() => {
      sendCurrentWeather(connectedDevice).catch(error => {
        console.error('Could not refresh weather:', error);
      });
    }, 30 * 60 * 1000);

    return () => clearInterval(interval);
  }, [connectedDevice, connectionStatus, sendCurrentWeather]);

  return { sendCurrentWeather };
}
