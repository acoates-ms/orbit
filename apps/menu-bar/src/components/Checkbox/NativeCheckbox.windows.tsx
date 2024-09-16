// Use switch instead of checkbox for now
import { Switch }  from 'react-native-windows';
import React from 'react';

import { CheckboxChangeEvent, NativeCheckboxProps } from './types';

const Checkbox = React.forwardRef(({ value, onChange }: NativeCheckboxProps, ref) => {
  return (
    <Switch
      value={Boolean(value)}
      style={{ marginLeft: -6 }}
      onChange={(event) =>
        onChange?.({
          nativeEvent: { value: event.nativeEvent.value },
        } as CheckboxChangeEvent)
      }
    />
  );
});

export default Checkbox;
