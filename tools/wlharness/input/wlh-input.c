// wlh-input: injects pointer and keyboard input into a KWin session through the
// org_kde_kwin_fake_input protocol. Used by the test harness to produce real
// compositor input events (required e.g. for Wayland drag & drop, which needs
// the serial of a genuine pointer button press).
//
// Usage: wlh-input COMMAND [ARGS...] [COMMAND [ARGS...]...]
//   move X Y              move the pointer to absolute logical coordinates
//   rmove DX DY           move the pointer relatively
//   down [BUTTON]         press a button (left, right, middle; default left)
//   up [BUTTON]           release a button
//   click [BUTTON]        press and release a button
//   drag X1 Y1 X2 Y2 [STEPS]
//                         press at (X1, Y1), move gradually to (X2, Y2), release
//   wheel DELTA           scroll vertically (positive = down)
//   key COMBO             press and release a key combination by keysym names,
//                         e.g. Return, ctrl+c, shift+Tab
//   type TEXT             type text character by character
//   sleep MS              wait
//
// WAYLAND_DISPLAY must point to the KWin session.

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "fake-input-client-protocol.h"

#define STEP_DELAY_MS 15
#define MAX_COMBO_KEYS 8

static struct org_kde_kwin_fake_input * FakeInput = NULL;
static uint32_t FakeInputVersion = 0;

static void RegistryGlobal(
  void * Data, struct wl_registry * Registry, uint32_t Name, const char * Interface, uint32_t Version)
{
  (void)Data;
  if (strcmp(Interface, org_kde_kwin_fake_input_interface.name) == 0)
  {
    FakeInputVersion = (Version < 6) ? Version : 6;
    FakeInput = wl_registry_bind(Registry, Name, &org_kde_kwin_fake_input_interface, FakeInputVersion);
  }
}

static void RegistryGlobalRemove(void * Data, struct wl_registry * Registry, uint32_t Name)
{
  (void)Data;
  (void)Registry;
  (void)Name;
}

static const struct wl_registry_listener RegistryListener = {
  .global = RegistryGlobal,
  .global_remove = RegistryGlobalRemove,
};

static void SleepMs(long Ms)
{
  struct timespec Ts = { Ms / 1000, (Ms % 1000) * 1000000L };
  while ((nanosleep(&Ts, &Ts) < 0) && (errno == EINTR))
  {
  }
}

static void Fail(const char * Format, const char * Arg)
{
  fprintf(stderr, "wlh-input: ");
  fprintf(stderr, Format, Arg);
  fprintf(stderr, "\n");
  exit(2);
}

static void Sync(struct wl_display * Display)
{
  if (wl_display_roundtrip(Display) < 0)
  {
    Fail("%s", "lost connection to the compositor");
  }
  SleepMs(STEP_DELAY_MS);
}

static double ParseNumber(const char * Text)
{
  char * End = NULL;
  double Result = strtod(Text, &End);
  if ((End == Text) || (*End != '\0'))
  {
    Fail("invalid number '%s'", Text);
  }
  return Result;
}

static uint32_t ParseButton(const char * Text)
{
  if (strcmp(Text, "left") == 0) return BTN_LEFT;
  if (strcmp(Text, "right") == 0) return BTN_RIGHT;
  if (strcmp(Text, "middle") == 0) return BTN_MIDDLE;
  Fail("unknown button '%s'", Text);
  return 0;
}

static bool IsButtonName(const char * Text)
{
  return (strcmp(Text, "left") == 0) || (strcmp(Text, "right") == 0) || (strcmp(Text, "middle") == 0);
}

static void MoveAbsolute(struct wl_display * Display, double X, double Y)
{
  org_kde_kwin_fake_input_pointer_motion_absolute(FakeInput, wl_fixed_from_double(X), wl_fixed_from_double(Y));
  Sync(Display);
}

static void Button(struct wl_display * Display, uint32_t ButtonCode, bool Pressed)
{
  org_kde_kwin_fake_input_button(
    FakeInput, ButtonCode, Pressed ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED);
  Sync(Display);
}

static void Keysym(struct wl_display * Display, xkb_keysym_t Sym, bool Pressed)
{
  org_kde_kwin_fake_input_keyboard_keysym(FakeInput, Sym, Pressed ? 1 : 0);
  Sync(Display);
}

static xkb_keysym_t KeysymFromName(const char * Name)
{
  static const struct { const char * Alias; const char * Name; } Aliases[] = {
    { "ctrl", "Control_L" }, { "shift", "Shift_L" }, { "alt", "Alt_L" }, { "super", "Super_L" },
    { "enter", "Return" }, { "esc", "Escape" }, { "del", "Delete" }, { "space", "space" },
  };
  for (size_t I = 0; I < sizeof(Aliases) / sizeof(Aliases[0]); I++)
  {
    if (strcasecmp(Name, Aliases[I].Alias) == 0)
    {
      Name = Aliases[I].Name;
      break;
    }
  }
  xkb_keysym_t Sym = xkb_keysym_from_name(Name, XKB_KEYSYM_NO_FLAGS);
  if (Sym == XKB_KEY_NoSymbol)
  {
    Sym = xkb_keysym_from_name(Name, XKB_KEYSYM_CASE_INSENSITIVE);
  }
  if (Sym == XKB_KEY_NoSymbol)
  {
    Fail("unknown key '%s'", Name);
  }
  return Sym;
}

static void KeyCombo(struct wl_display * Display, const char * Combo)
{
  char Buffer[256];
  snprintf(Buffer, sizeof(Buffer), "%s", Combo);
  xkb_keysym_t Syms[MAX_COMBO_KEYS];
  int Count = 0;
  for (char * Save = NULL, * Part = strtok_r(Buffer, "+", &Save); Part != NULL; Part = strtok_r(NULL, "+", &Save))
  {
    if (Count == MAX_COMBO_KEYS)
    {
      Fail("too many keys in '%s'", Combo);
    }
    Syms[Count++] = KeysymFromName(Part);
  }
  for (int I = 0; I < Count; I++)
  {
    Keysym(Display, Syms[I], true);
  }
  for (int I = Count - 1; I >= 0; I--)
  {
    Keysym(Display, Syms[I], false);
  }
}

static void TypeText(struct wl_display * Display, const char * Text)
{
  // Decode UTF-8 and send each code point as a keysym
  const unsigned char * P = (const unsigned char *)Text;
  while (*P != '\0')
  {
    uint32_t CodePoint;
    int Length;
    if (*P < 0x80) { CodePoint = *P; Length = 1; }
    else if ((*P >> 5) == 0x6) { CodePoint = *P & 0x1F; Length = 2; }
    else if ((*P >> 4) == 0xE) { CodePoint = *P & 0x0F; Length = 3; }
    else if ((*P >> 3) == 0x1E) { CodePoint = *P & 0x07; Length = 4; }
    else { Fail("%s", "invalid UTF-8 in text"); return; }
    for (int I = 1; I < Length; I++)
    {
      if ((P[I] & 0xC0) != 0x80)
      {
        Fail("%s", "invalid UTF-8 in text");
      }
      CodePoint = (CodePoint << 6) | (P[I] & 0x3F);
    }
    P += Length;

    xkb_keysym_t Sym;
    if (CodePoint == '\n') Sym = XKB_KEY_Return;
    else if (CodePoint == '\t') Sym = XKB_KEY_Tab;
    else Sym = xkb_utf32_to_keysym(CodePoint);
    if (Sym == XKB_KEY_NoSymbol)
    {
      Fail("%s", "character has no keysym");
    }
    Keysym(Display, Sym, true);
    Keysym(Display, Sym, false);
  }
}

static void Drag(struct wl_display * Display, double X1, double Y1, double X2, double Y2, int Steps)
{
  MoveAbsolute(Display, X1, Y1);
  Button(Display, BTN_LEFT, true);
  // Small initial moves so the source crosses its drag start threshold
  // before the pointer leaves the source surface
  for (int I = 1; I <= Steps; I++)
  {
    double T = (double)I / Steps;
    MoveAbsolute(Display, X1 + (X2 - X1) * T, Y1 + (Y2 - Y1) * T);
    SleepMs(STEP_DELAY_MS);
  }
  // Let the target process the enter/motion events before the drop
  SleepMs(200);
  Button(Display, BTN_LEFT, false);
}

static const char * NextArg(int Argc, char ** Argv, int * Index, const char * Command)
{
  if (*Index >= Argc)
  {
    Fail("missing argument for '%s'", Command);
  }
  return Argv[(*Index)++];
}

int main(int Argc, char ** Argv)
{
  if (Argc < 2)
  {
    fprintf(stderr, "usage: wlh-input COMMAND [ARGS...] [COMMAND [ARGS...]...]\n");
    return 2;
  }

  struct wl_display * Display = wl_display_connect(NULL);
  if (Display == NULL)
  {
    Fail("%s", "cannot connect to the Wayland display (is WAYLAND_DISPLAY set?)");
  }
  struct wl_registry * Registry = wl_display_get_registry(Display);
  wl_registry_add_listener(Registry, &RegistryListener, NULL);
  wl_display_roundtrip(Display);
  if (FakeInput == NULL)
  {
    Fail("%s", "the compositor does not offer org_kde_kwin_fake_input");
  }
  if (FakeInputVersion < 6)
  {
    Fail("%s", "org_kde_kwin_fake_input version 6 or newer is required");
  }
  org_kde_kwin_fake_input_authenticate(FakeInput, "wlh-input", "LinSCP test harness");
  Sync(Display);

  int Index = 1;
  while (Index < Argc)
  {
    const char * Command = Argv[Index++];
    if (strcmp(Command, "move") == 0)
    {
      double X = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      double Y = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      MoveAbsolute(Display, X, Y);
    }
    else if (strcmp(Command, "rmove") == 0)
    {
      double DX = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      double DY = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      org_kde_kwin_fake_input_pointer_motion(FakeInput, wl_fixed_from_double(DX), wl_fixed_from_double(DY));
      Sync(Display);
    }
    else if ((strcmp(Command, "down") == 0) || (strcmp(Command, "up") == 0) || (strcmp(Command, "click") == 0))
    {
      uint32_t ButtonCode = BTN_LEFT;
      if ((Index < Argc) && IsButtonName(Argv[Index]))
      {
        ButtonCode = ParseButton(Argv[Index++]);
      }
      if (strcmp(Command, "up") != 0)
      {
        Button(Display, ButtonCode, true);
      }
      if (strcmp(Command, "down") != 0)
      {
        Button(Display, ButtonCode, false);
      }
    }
    else if (strcmp(Command, "drag") == 0)
    {
      double X1 = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      double Y1 = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      double X2 = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      double Y2 = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      int Steps = 20;
      if ((Index < Argc) && (strspn(Argv[Index], "0123456789") == strlen(Argv[Index])))
      {
        Steps = atoi(Argv[Index++]);
        if (Steps < 1)
        {
          Steps = 1;
        }
      }
      Drag(Display, X1, Y1, X2, Y2, Steps);
    }
    else if (strcmp(Command, "wheel") == 0)
    {
      double Delta = ParseNumber(NextArg(Argc, Argv, &Index, Command));
      org_kde_kwin_fake_input_axis(FakeInput, WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(Delta));
      Sync(Display);
    }
    else if (strcmp(Command, "key") == 0)
    {
      KeyCombo(Display, NextArg(Argc, Argv, &Index, Command));
    }
    else if (strcmp(Command, "type") == 0)
    {
      TypeText(Display, NextArg(Argc, Argv, &Index, Command));
    }
    else if (strcmp(Command, "sleep") == 0)
    {
      SleepMs((long)ParseNumber(NextArg(Argc, Argv, &Index, Command)));
    }
    else
    {
      Fail("unknown command '%s'", Command);
    }
  }

  org_kde_kwin_fake_input_destroy(FakeInput);
  wl_registry_destroy(Registry);
  wl_display_roundtrip(Display);
  wl_display_disconnect(Display);
  return 0;
}
