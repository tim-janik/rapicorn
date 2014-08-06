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
          <TextEditor> <Label markup-text="@bind name"/> </TextEditor>
        </HBox>
      </VBox>
    </Alignment>
  </tmpl:define>
"""

app.load_string ("T", hello_window) # useless namespace

window = app.create_window ("T:testbind-py")

# window.data_context (None) # FIXME

window.show()
app.loop()
