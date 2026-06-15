/*
 * SQGI Autostart Demo
 *
 * A GTK4 Squirrel app that toggles the GAutostart GI library.
 */

local GLib = import("GLib")
local Gio = import("Gio")
local Gtk = import("Gtk", "4.0")
local GAutostart = import("GAutostart", "1.0")
local system = import("system")

const APP_ID = "io.github.supercamel.GAutostartSqgiDemo"
const APP_NAME = "SQGI Autostart Demo"
local PACKAGE_ICON_NAME = APP_ID

local UI = {
    window = null,
    checkbox = null,
    status = null,
    command = null,
    busy = false,
    loading = false,
}

function usage() {
    print("Usage:\n")
    print("  sqgi main.nut                  Open the GTK4 app\n")
    print("  sqgi main.nut --auto-close=MS  Open the GTK4 app briefly for smoke tests\n")
    print("  sqgi main.nut --register       Register the packaged/default launch command\n")
    print("  sqgi main.nut --register-cmd COMMAND [ARGS...]\n")
    print("  sqgi main.nut --unregister\n")
    print("  sqgi main.nut --autostart      Record startup and open the GTK4 app\n")
    print("  sqgi main.nut --started        Record that autostart launched the app\n")
    print("  sqgi main.nut --status-path\n")
    print("  sqgi main.nut --print-default-command\n")
}

function file_exists(path) {
    return path != null && Gio.File.new_for_path(path).query_exists(null)
}

function first_existing_path(paths) {
    foreach (path in paths)
        if (file_exists(path)) return path
    return null
}

function icon_path() {
    local candidates = []

    if (system.paths.app_resources != null)
        candidates.append(GLib.build_filenamev([system.paths.app_resources, "assets", "sqgi_icon.png"]))

    local cwd = GLib.get_current_dir()
    candidates.append(GLib.build_filenamev([cwd, "assets", "sqgi_icon.png"]))
    candidates.append(GLib.build_filenamev([cwd, "demo", "sqgi-autostart", "assets", "sqgi_icon.png"]))

    return first_existing_path(candidates)
}

function quote_for_display(args) {
  local out = []
    foreach (arg in args) {
        if (arg.find(" ") == null && arg.find("\"") == null) {
            out.append(arg)
            continue
        }

        local escaped = ""
        for (local i = 0; i < arg.len(); i++) {
            local ch = arg.slice(i, i + 1)
            if (ch == "\"" || ch == "\\") escaped += "\\"
            escaped += ch
        }
        out.append("\"" + escaped + "\"")
    }

    local joined = ""
    for (local i = 0; i < out.len(); i++) {
        if (i > 0) joined += " "
        joined += out[i]
    }
    return joined
}

function status_path() {
    local cache_dir = system.paths.cache
    if (cache_dir == null) cache_dir = GLib.get_tmp_dir()
    return GLib.build_filenamev([cache_dir, "gautostart-sqgi-demo-started.log"])
}

function ensure_parent_dir(path) {
    local parent = GLib.path_get_dirname(path)
    GLib.mkdir_with_parents(parent, 448) // 0700
}

function append_started_log() {
    local path = status_path()
    ensure_parent_dir(path)

    local now = GLib.DateTime.new_now_local()
    local line = now.format("%Y-%m-%d %H:%M:%S %Z") + " " + APP_NAME + " was launched\n"
    local old = ""
    local file = Gio.File.new_for_path(path)

    if (file.query_exists(null)) {
        local loaded = file.load_contents(null)
        old = (typeof(loaded) == "array") ? loaded[0] : loaded
    }

    GLib.file_set_contents(path, old + line, -1)
    return path
}

function script_path_from_app_share() {
    if (system.paths.app_share == null) return null

    local compiled = GLib.build_filenamev([system.paths.app_share, "main.cnut"])
    if (file_exists(compiled)) return compiled

    local source = GLib.build_filenamev([system.paths.app_share, "main.nut"])
    if (file_exists(source)) return source

    return null
}

function local_script_path() {
    local cwd = GLib.get_current_dir()
    return first_existing_path([
        GLib.build_filenamev([cwd, "main.nut"]),
        GLib.build_filenamev([cwd, "demo", "sqgi-autostart", "main.nut"]),
    ])
}

function default_command() {
    local appimage = system.env.get("APPIMAGE")
    if (appimage != null && appimage != "")
        return [appimage, "--autostart"]

    local script = script_path_from_app_share()
    if (script == null) script = local_script_path()
    if (script == null) script = "demo/sqgi-autostart/main.nut"

    local sqgi_bin = null
    local appdir = system.env.get("SQGI_APPDIR")
    if (appdir != null && appdir != "") {
        if (system.os.family == "windows")
            sqgi_bin = GLib.build_filenamev([appdir, "bin", "sqgi.exe"])
        else
            sqgi_bin = GLib.build_filenamev([appdir, "usr", "bin", "sqgi"])
    }

    if (sqgi_bin == null || sqgi_bin == "")
        sqgi_bin = GLib.find_program_in_path("sqgi")
    if (sqgi_bin == null)
        sqgi_bin = "sqgi"

    return [sqgi_bin, script, "--autostart"]
}

function manager() {
    return GAutostart.Manager.new(APP_ID, APP_NAME)
}

function is_registered() {
    return manager().is_registered()
}

function register_autostart(command_line) {
    local m = manager()
    return sqgi.gio_async(
        @(cancellable, cb) m.register(command_line,
                                      "Launch the SQGI autostart demo at login",
                                      cancellable,
                                      cb),
        @(res) m.register_finish(res)
    )
}

function unregister_autostart() {
    local m = manager()
    return sqgi.gio_async(
        @(cancellable, cb) m.unregister(cancellable, cb),
        @(res) m.unregister_finish(res)
    )
}

function set_status(message) {
    if (UI.status != null) UI.status.set_text(message)
}

function refresh_state() {
    if (UI.checkbox == null) return

    local enabled = false
    try {
        enabled = is_registered()
        set_status(enabled ? "Autostart is enabled." : "Autostart is disabled.")
    } catch (e) {
        set_status("Could not read autostart state: " + e)
    }

    UI.loading = true
    UI.checkbox.set_active(enabled)
    UI.loading = false

    if (UI.command != null)
        UI.command.set_text(quote_for_display(default_command()))
}

function set_busy(busy) {
    UI.busy = busy
    if (UI.checkbox != null) UI.checkbox.set_sensitive(!busy)
}

async function set_autostart_enabled(enabled) {
    set_busy(true)
    try {
        if (enabled) {
            set_status("Registering autostart...")
            await register_autostart(default_command())
            set_status("Autostart is enabled.")
        } else {
            set_status("Unregistering autostart...")
            await unregister_autostart()
            set_status("Autostart is disabled.")
        }
    } catch (e) {
        set_status("Autostart change failed: " + e)
    }

    set_busy(false)
    refresh_state()
}

function make_icon_widget() {
    local path = icon_path()
    local widget = null

    if (path != null)
        widget = Gtk.Picture.new_for_filename(path)
    else
        widget = Gtk.Image.new_from_icon_name(PACKAGE_ICON_NAME)

    widget.set_size_request(72, 72)
    return widget
}

function build_window(app) {
    local win = Gtk.ApplicationWindow.new(app)
    win.set_title(APP_NAME)
    win.set_default_size(520, 320)
    win.set_icon_name(PACKAGE_ICON_NAME)
    UI.window = win

    local header = Gtk.HeaderBar.new()
    header.set_title_widget(Gtk.Label.new(APP_NAME))
    win.set_titlebar(header)

    local root = Gtk.Box.new(Gtk.Orientation.vertical, 16)
    root.set_margin_top(18)
    root.set_margin_bottom(18)
    root.set_margin_start(18)
    root.set_margin_end(18)
    win.set_child(root)

    local intro = Gtk.Box.new(Gtk.Orientation.horizontal, 14)
    intro.append(make_icon_widget())

    local intro_text = Gtk.Box.new(Gtk.Orientation.vertical, 4)
    local title = Gtk.Label.new(APP_NAME)
    title.set_xalign(0.0)
    title.add_css_class("title-2")
    local subtitle = Gtk.Label.new("Toggle whether this app starts when you log in.")
    subtitle.set_xalign(0.0)
    subtitle.set_wrap(true)
    intro_text.append(title)
    intro_text.append(subtitle)
    intro.append(intro_text)
    root.append(intro)

    local check = Gtk.CheckButton.new_with_label("Start automatically when I log in")
    check.connect("toggled", function() {
        if (UI.loading || UI.busy) return
        set_autostart_enabled(check.get_active())
    })
    UI.checkbox = check
    root.append(check)

    local command_label = Gtk.Label.new("Launch command")
    command_label.set_xalign(0.0)
    command_label.add_css_class("heading")
    root.append(command_label)

    local command = Gtk.Label.new("")
    command.set_xalign(0.0)
    command.set_wrap(true)
    command.add_css_class("monospace")
    UI.command = command
    root.append(command)

    local actions = Gtk.Box.new(Gtk.Orientation.horizontal, 8)
    local refresh = Gtk.Button.new_with_label("Refresh")
    refresh.connect("clicked", function() { refresh_state() })
    actions.append(refresh)

    local simulate = Gtk.Button.new_with_label("Run startup action now")
    simulate.connect("clicked", function() {
        local path = append_started_log()
        set_status("Startup action wrote " + path)
    })
    actions.append(simulate)
    root.append(actions)

    local status = Gtk.Label.new("")
    status.set_xalign(0.0)
    status.set_wrap(true)
    UI.status = status
    root.append(status)

    refresh_state()
    return win
}

function run_gui(auto_close_ms) {
    local app = Gtk.Application.new(APP_ID, Gio.ApplicationFlags.flags_none)

    app.connect("activate", function() {
        if (UI.window == null)
            build_window(app)
        UI.window.present()

        if (auto_close_ms > 0) {
            sqgi.timeout_add(auto_close_ms, function() {
                app.quit()
                return false
            })
        }
    })

    return app.run(0, null)
}

async function run_cli() {
    local op = vargv[0]

    if (op == "--status-path") {
        print(status_path() + "\n")
        return 0
    }

    if (op == "--print-default-command") {
        print(quote_for_display(default_command()) + "\n")
        return 0
    }

    if (op == "--started") {
        print("wrote " + append_started_log() + "\n")
        return 0
    }

    if (op == "--register") {
        local command_line = default_command()
        print("registering autostart command:\n  " + quote_for_display(command_line) + "\n")
        await register_autostart(command_line)
        print("registered\n")
        return 0
    }

    if (op == "--register-cmd") {
        if (vargv.len() < 2)
            throw "--register-cmd requires COMMAND [ARGS...]"

        local command_line = []
        for (local i = 1; i < vargv.len(); i++)
            command_line.append(vargv[i])

        print("registering autostart command:\n  " + quote_for_display(command_line) + "\n")
        await register_autostart(command_line)
        print("registered\n")
        return 0
    }

    if (op == "--unregister") {
        await unregister_autostart()
        print("unregistered\n")
        return 0
    }

    throw "unknown option: " + op
}

if (vargv.len() == 0 || vargv[0].find("--auto-close=") == 0 || vargv[0] == "--autostart") {
    local auto_close_ms = 0
    if (vargv.len() > 0)
        if (vargv[0].find("--auto-close=") == 0)
            auto_close_ms = vargv[0].slice(13).tointeger()

    if (vargv.len() > 0 && vargv[0] == "--autostart")
        print("wrote " + append_started_log() + "\n")

    local status = run_gui(auto_close_ms)
    if (status != 0) throw "GTK application exited with status " + status
} else if (vargv[0] == "--help" || vargv[0] == "-h") {
    usage()
} else {
    local loop = GLib.MainLoop.new(null, false)
    local exit_code = 0

    run_cli().then(function(code) {
        exit_code = code
        loop.quit()
    }, function(e) {
        ::print("error: " + e + "\n")
        exit_code = 1
        loop.quit()
    })

    loop.run()

    if (exit_code != 0)
        throw "SQGI Autostart Demo failed"
}
