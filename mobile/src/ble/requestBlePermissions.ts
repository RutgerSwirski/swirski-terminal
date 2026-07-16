import { PermissionsAndroid, Platform } from 'react-native';

export async function requestBlePermissions(): Promise<boolean> {
  if (Platform.OS !== 'android') {
    return true;
  }

  if (Platform.Version >= 31) {
    const results = await PermissionsAndroid.requestMultiple([
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
      PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
    ]);

    return (
      results['android.permission.BLUETOOTH_SCAN'] ===
        PermissionsAndroid.RESULTS.GRANTED &&
      results['android.permission.BLUETOOTH_CONNECT'] ===
        PermissionsAndroid.RESULTS.GRANTED
    );
  }

  const result = await PermissionsAndroid.request(
    PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
  );

  return result === PermissionsAndroid.RESULTS.GRANTED;
}
