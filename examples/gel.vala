class Demo : Object {
    ValueArray array;
    Gel.Context context;

    public void quit(Closure closure, out Value? return_value) {
        if(return_value != null)
            return_value = "Bye!";
        Gtk.main_quit();
    }

    public Demo() {
        print("Instantiating a %s\n", typeof(Gel.Context).name());
        context = new Gel.Context();
        context.insert_function("quit", quit);
    }

    public bool run() {
        foreach(Value iter_value in array.values) {
            print("\n%s ?\n", Gel.Value.to_string(iter_value));

            GLib.Value result_value;
            if(context.eval(iter_value, out result_value))
                print("= %s\n\n", Gel.Value.to_string(result_value));
        }
        return true;
    }

    public void parse(string file) throws FileError {
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

    var demo = new Demo();

    try {
        demo.parse(args[1]);
    }
    catch {
        print("Could not parse file '%s'\n", args[1]);
        return 1;
    }

    Gtk.init_add(demo.run);
    Gtk.main();
    return 0;
}

