import { useEffect } from 'react';

import FileHandler from '../../../modules/file-handler';

export type UseFileHandlerParams = {
  onOpenFile: (path: string) => void;
};

export const useFileHandler = ({ onOpenFile }: UseFileHandlerParams) => {
  useEffect(() => {
    const subscription = FileHandler.onOpenFile(({ path }: { path: string }) => {
      onOpenFile(path);
    });

    return subscription.remove;
  }, [onOpenFile]);
};
