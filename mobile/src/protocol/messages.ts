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
    body: 'This should appear as a toast on the ESP32.',
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
