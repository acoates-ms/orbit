import AutoUpdater from './src/AutoUpdaterModule';

export default {
  ...AutoUpdater,
  checkForUpdates: () => AutoUpdater.checkForUpdates(),
  setAutomaticallyChecksForUpdates: (value: boolean) =>
    AutoUpdater.setAutomaticallyChecksForUpdates(value),
  getAutomaticallyChecksForUpdates: () => AutoUpdater.getAutomaticallyChecksForUpdates(),
};
