# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
# [HelloRapicorn-EXAMPLE]
import Rapicorn # Load Rapicorn language bindings for Python


# Retrieve the global Rapicorn Application object
# and setup a user digestible application name.
app = Rapicorn.init_app ("Hello Rapicorn")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <Window id="hello-window">
    <Alignment padding="15">
      <VBox spacing="30">
        <Label markup-text="Hello World!"/>
        <Button on-click="CLICK">
          <Label markup-text="Close" />
        </Button>
      </VBox>
    </Alignment>
  </Window>
"""

# Register the 'hello-window' definition for later use, for this we need
# a unique domain string, it's easiest to reuse the application name.
app.load_string (hello_window)

# The above is all that is needed to allow us to create the window object.
window = app.create_window ("hello-window")

# This function is called to handle the command we use for button clicks.
def command_handler (command_name, args):
  # When we see the 'CLICK' command, close down the Application
  if command_name == "CLICK":
    app.close_all();
  return True # handled

# Call the handler when the Window::commands signal is emitted.
window.sig_commands.connect (command_handler)

# Preparations done, now it's time to show the window on the screen.
window.show()

# Pass control to the event loop, to wait and handle user commands.
app.run()
# [HelloRapicorn-EXAMPLE]

# Test code, run this with above app.run disabled, exits 123 if OK
import sys
if '--test-123' in sys.argv:
  button = window.query_selector_unique ('.Button')
  assert button
  ok = window.synthesize_enter()
  assert ok
  ok = window.synthesize_click (button, 1)
  assert ok
  ok = window.synthesize_leave()
  assert ok
  app.run()
  sys.exit (123)
