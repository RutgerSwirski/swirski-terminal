import { TerminalNotification } from '../ble/types';

export function createPingMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'ping',
    id: `mobile-${Date.now()}`,
    payload: {
      debugText: 'A'.repeat(300),
    },
  };
}

export function createDisconnectMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'disconnect.requested',
    id: `mobile-disconnect-${Date.now()}`,
  };
}

export function createTimeSyncMessage(): Record<string, unknown> {
  const now = new Date();

  return {
    version: 1,
    type: 'time.sync',
    id: `mobile-time-${Date.now()}`,
    payload: {
      unixTimeSeconds: Math.floor(now.getTime() / 1000),
      timezoneOffsetMinutes: -now.getTimezoneOffset(),
    },
  };
}

export function createWifiScanRequestMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'wifi.scan.request',
    id: `mobile-wifi-scan-${Date.now()}`,
  };
}

export function createWifiConfigureMessage(
  ssid: string,
  password: string,
): Record<string, unknown> {
  return {
    version: 1,
    type: 'wifi.configure',
    id: `mobile-wifi-configure-${Date.now()}`,
    payload: {
      ssid,
      password,
    },
  };
}

export function createWifiInternetTestMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'wifi.internet.test',
    id: `mobile-wifi-internet-test-${Date.now()}`,
  };
}

export function createWifiDisconnectMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'wifi.disconnect',
    id: `mobile-wifi-disconnect-${Date.now()}`,
  };
}

export function createTestNotificationSnapshotMessage(): Record<
  string,
  unknown
> {
  const notifications: TerminalNotification[] = [
    {
      id: 'test-whatsapp-1',
      packageName: 'com.whatsapp',
      appName: 'WhatsApp',
      title: 'Stella',
      body: 'Are you still coming tonight?',
      postedAt: Date.now(),
    },
    {
      id: 'test-calendar-1',
      packageName: 'com.google.android.calendar',
      appName: 'Calendar',
      title: 'Design meeting testing!',
      body: 'Starts in 10 minutes',
      postedAt: Date.now() - 10_000,
    },
  ];

  return {
    version: 1,
    type: 'notifications.snapshot',
    id: `mobile-snapshot-${Date.now()}`,
    payload: {
      notifications,
    },
  };
}

export function createTestNotificationReceivedMessage(): Record<
  string,
  unknown
> {
  const notification: TerminalNotification = {
    id: `test-live-notification-${Date.now()}`,
    packageName: 'com.whatsapp.android',
    appName: 'WhatsApp',
    title: 'Test live notification',
    body: 'This should appear as a toast on the ESP32.'.repeat(10),
    postedAt: Date.now(),
  };

  return {
    version: 1,
    type: 'notification.received',
    id: `mobile-notification-${Date.now()}`,
    payload: {
      notification,
    },
  };
}

export function createTestMusicStateMessage(): Record<string, unknown> {
  return {
    version: 1,
    type: 'music.state',
    id: `mobile-music-${Date.now()}`,
    payload: {
      appName: 'Spotify',
      title: 'Once in a Lifetime',
      artist: 'Talking Heads',
      isPlaying: true,
    },
  };
}
