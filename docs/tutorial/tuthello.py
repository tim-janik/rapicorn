# [HelloRapicorn-EXAMPLE]
# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
import Rapicorn1303 as Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Hello Rapicorn")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <tmpl:define id="hello-window" inherit="Window">
    <Alignment padding="15">
      <VBox spacing="30">
        <Label markup-text="Hello World!"/>
        <Button on-click="CLICK">
          <Label markup-text="Close" />
        </Button>
      </VBox>
    </Alignment>
  </tmpl:define>
"""

# Register the 'hello-window' definition for later use, for this we need
# a unique domain string, it's easiest to reuse the application name.
app.load_string ("HelloRapicorn", hello_window)

# The above is all that is needed to allow us to create the window object.
window = app.create_window ("HelloRapicorn:hello-window")

# This function is called to handle the command we use for button clicks.
def command_handler (command_name, args):
  # When we see the 'CLICK' command, close down the Application
  if command_name == "CLICK":
    app.close_all();

# Call the handler when the Window::commands signal is emitted.
window.sig_commands_connect (command_handler)

# Preparations done, now it's time to show the window on the screen.
window.show()

# Pass control to the event loop, to wait and handle user commands.
app.loop()
# [HelloRapicorn-EXAMPLE]
