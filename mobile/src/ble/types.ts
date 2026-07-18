export type TerminalNotification = {
  id: string;
  packageName: string;
  appName: string;
  title: string;
  body: string;
  postedAt: number;
};

export type NotificationsSnapshotMessage = {
  version: 1;
  type: 'notifications.snapshot';
  id: string;
  payload: {
    notifications: TerminalNotification[];
  };
};

export type NotificationReceivedMessage = {
  version: 1;
  type: 'notification.received';
  id: string;
  payload: {
    notification: TerminalNotification;
  };
};

export type NotificationRemovedMessage = {
  version: 1;
  type: 'notification.removed';
  id: string;
  payload: {
    id: string;
  };
};
