# Load and import a versioned Rapicorn module into the 'Rapicorn' namespace
from Rapicorn1307 import Rapicorn

# Setup the application object, unsing a unique application name.
app = Rapicorn.app_init ("Test Rapicorn Bindings")

# Define the elements of the dialog window to be displayed.
hello_window = """
  <tmpl:define id="testbind-py" inherit="Window">
    <Alignment padding="15">
      <VBox spacing="30">
        <HBox>
          <Label markup-text="Enter Name:"/>
          <TextEditor> <Label markup-text="@bind title"/> </TextEditor>
        </HBox>
      </VBox>
    </Alignment>
  </tmpl:define>
"""

app.load_string ("T", hello_window) # useless namespace

window = app.create_window ("T:testbind-py")

@Rapicorn.Bindable
class ObjectModel (object):
  def __init__ (self):
    self.title = "Hello There!"
om = ObjectModel()

window.data_context (om.__aida_relay__)

window.show()
app.loop()
