class Demo : Object {
    ValueArray array;
    Gel.Context context;

    public Demo() {
        context = new Gel.Context();
        context.add_callback("quit", Gtk.main_quit);
    }

    public bool run() {
        foreach(Value iter_value in array.values) {
            print("%s ?\n", iter_value.strdup_contents());

            GLib.Value result_value;
            if(context.eval(iter_value, out result_value))
                print("= %s\n\n", result_value.strdup_contents());
        }
        return true;
    }

    public void parse_file(string file) throws FileError {
        array = Gel.parse_file(file);
    }
}

int main(string[] args) {
    Gtk.init(ref args);

    Type type;
    type = typeof(Gtk.Window);
    type = typeof(Gtk.VBox);
    type = typeof(Gtk.HBox);
    type = typeof(Gtk.Entry);
    type = typeof(Gtk.Button);

    if(args.length != 2) {
        printerr("%s requires an argument\n", args[0]);
        return 1;
    }

    unowned string file = args[1];

    Demo demo = new Demo();

    try {
        demo.parse_file(file);
    }
    catch {
        print("Could not parse file '%s'\n", file);
        return 1;
    }

    Gtk.init_add(demo.run);
    Gtk.main();
    return 0;
}

