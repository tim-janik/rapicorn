# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
from Rapicorn1307 import Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <tmpl:define id="example-bind-py" inherit="Window">
    <Alignment padding="15">
      <VBox spacing="30">
        <HBox>
          <Label markup-text="Editable Text: "/>
          <TextEditor> <Label markup-text="@bind title"/> </TextEditor>
        </HBox>
        <HBox homogeneous="true" spacing="15">
          <Button on-click="show" hexpand="1">    <Label markup-text="Show Model"/> </Button>
          <Button on-click="shuffle"> <Label markup-text="Shuffle Model"/> </Button>
        </HBox>
      </VBox>
    </Alignment>
  </tmpl:define>
"""

app.load_string ("T", hello_window) # useless namespace

window = app.create_window ("T:example-bind-py")

@Rapicorn.Bindable
class ObjectModel (object):
  def __init__ (self):
    self.title = "Hello There!"
om = ObjectModel()

window.data_context (om.__aida_relay__)

def window_command_handler (cmdname, args):
  if cmdname == 'show':
    print 'ObjectModel.title:', om.title
  if cmdname == 'shuffle':
    import random
    v = 'Random bits:' + str (random.randrange (1000000))
    om.title = v
  return True

window.sig_commands += window_command_handler
window.show()
app.loop()
