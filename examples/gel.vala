class Demo : Gel.Context {
    public ValueArray array;

    public Demo() {
        quit.connect(Gtk.main_quit);
    }

    public bool init()
    {
        uint i;
        for(i = 0; i < array.values.length; i++)
        {
            string value_string = array.values[i].strdup_contents();
            string escaped_string = value_string.escape("");
            print("%s ?\n", escaped_string);

            GLib.Value result_value;
            if(eval(array.values[i], out result_value))
            {
                value_string = result_value.strdup_contents();
                print("= %s\n\n", value_string);
            }
        }
        return true;
    }

    public void parse_file(string file) throws FileError {
        array = Gel.parse_file(file);
    }
}

int main(string[] args)
{
    Gtk.init(ref args);

    Type type;
    type = typeof(Gtk.Window);
    type = typeof(Gtk.VBox);
    type = typeof(Gtk.HBox);
    type = typeof(Gtk.Entry);
    type = typeof(Gtk.Button);

    if(args.length != 2)
    {
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

    Gtk.init_add(demo.init);
    Gtk.main();
    return 0;
}

