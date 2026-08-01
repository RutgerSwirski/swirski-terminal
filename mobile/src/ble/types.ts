export type TerminalNotification = {
  id: string;
  packageName: string;
  appName: string;
  title: string;
  body: string;
  postedAt: number;
  ongoing: boolean;
  category: string | null;
};

export type NotificationsSnapshotMessage = {
  version: 1;
  type: 'notifications.snapshot';
  id: string;
  payload: {
    notifications: TerminalNotification[];
  };
};

export type NotificationUpsertedMessage = {
  version: 1;
  type: 'notification.upserted';
  id: string;
  payload: {
    alert: boolean;
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
