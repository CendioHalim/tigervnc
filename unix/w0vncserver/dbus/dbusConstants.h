#ifndef __DBUS_CONSTANTS_H__
#define __DBUS_CONSTANTS_H__

namespace dbus {
  /* -=- org.freedesktop.RemoteDesktop1 constants */
  // AccessModes
  static const int ACCESS_MODE_HEADLESS = 1;
  static const int ACCESS_MODE_HYBRID = 2;

  // Cursor modes
  static const int CUR_HIDDEN =   1;
  static const int CUR_EMBEDDED = 2;
  static const int CUR_METADATA = 3;

  // Keymap types
  static const int KEYMAP_TYPE_XKB = 0;

  // Keymap format
  static const int KEYMAP_FORMAT_XKB_V1 = 1;
  static const int KEYMAP_FORMAT_XKB_V2 = 2;

};

#endif // __DBUS_CONSTANTS_H__
