# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
# Licensed CC0 Public Domain: http://creativecommons.org/publicdomain/zero/1.0
import Rapicorn # Load Rapicorn language bindings for Python

# Setup the application object, unsing a unique application name.
app = Rapicorn.init_app ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <Window declare="example-bind-py">
    <Alignment padding="15">
      <VBox spacing="30">
        <HBox id="binding-box">
          <Label markup-text="Editable Text: "/>
          <!-- [EXAMPLE-bind-title] -->
          <TextEditor markup-text="@bind title"/>
          <!-- [EXAMPLE-bind-title] -->
        </HBox>
        <HBox homogeneous="true" spacing="15">
          <Button on-click="show" hexpand="1">    <Label markup-text="Show Model"/> </Button>
          <Button id="shuffler" on-click="shuffle"> <Label markup-text="Shuffle Model"/> </Button>
        </HBox>
      </VBox>
    </Alignment>
  </Window>
"""

app.load_string (hello_window)

window = app.create_window ("example-bind-py")

# [EXAMPLE-Bindable-ObjectModel]
@Rapicorn.Bindable
class ObjectModel (object):
  def __init__ (self):
    self.title = "Hello There!"
om = ObjectModel()
window.data_context (om.__aida_relay__)
# [EXAMPLE-Bindable-ObjectModel]

def window_command_handler (cmdname, args):
  if cmdname == 'show':
    print 'ObjectModel.title:', om.title
  if cmdname == 'shuffle':
    import random
    v = 'Random bits:' + str (random.randrange (1000000))
    om.title = v
  return True

window.sig_commands.connect (window_command_handler)
window.show()

import sys
if not '-i' in sys.argv:
  editor = window.query_selector_unique ('#binding-box .TextEditor')
  assert editor
  window.synthesize_enter()
  app.process_events (window)           # allow event processing
  initial_text = editor.markup_text
  window.synthesize_click (window.query_selector_unique ("#shuffler"), 1)
  app.process_events (window)           # allow event processing
  after_click_text = editor.markup_text
  assert initial_text != after_click_text
  window.synthesize_delete()

app.run()
