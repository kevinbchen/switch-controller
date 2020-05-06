import pygame
import serial
import time


bluetooth_serial = serial.Serial('COM10', 9600, timeout=0)
#print("connected!")
#input("wait")




# Define some colors.
BLACK = pygame.Color('black')
WHITE = pygame.Color('white')


# This is a simple class that will help us print to the screen.
# It has nothing to do with the joysticks, just outputting the
# information.
class TextPrint(object):
    def __init__(self):
        self.reset()
        self.font = pygame.font.Font(None, 20)

    def tprint(self, screen, textString):
        textBitmap = self.font.render(textString, True, BLACK)
        screen.blit(textBitmap, (self.x, self.y))
        self.y += self.line_height

    def reset(self):
        self.x = 10
        self.y = 10
        self.line_height = 15

    def indent(self):
        self.x += 10

    def unindent(self):
        self.x -= 10

def axis_float_to_byte(value):
    int_val = min(max(0, int(value * 128 + 128)), 254)
    if abs(int_val - 128) < 16:
        int_val = 128
    return int_val


pygame.init()
screen = pygame.display.set_mode((500, 700))

pygame.joystick.init()

xbox_joystick = None
for i in range(pygame.joystick.get_count()):
    joystick = pygame.joystick.Joystick(i)
    joystick.init()
    print(joystick.get_name())
    if "xbox" in joystick.get_name().lower():
        xbox_joystick = joystick
joystick = xbox_joystick

clock = pygame.time.Clock()
done = False


textPrint = TextPrint()

last_poll_time = None
poll_delays = []

# -------- Main Program Loop -----------
while not done:
    #
    # EVENT PROCESSING STEP
    #
    # Possible joystick actions: JOYAXISMOTION, JOYBALLMOTION, JOYBUTTONDOWN,
    # JOYBUTTONUP, JOYHATMOTION
    for event in pygame.event.get(): # User did something.
        if event.type == pygame.QUIT: # If user clicked close.
            done = True # Flag that we are done so we exit this loop.
        elif event.type == pygame.JOYBUTTONDOWN:
            print("Joystick button pressed.")
        elif event.type == pygame.JOYBUTTONUP:
            print("Joystick button released.")

    #
    # DRAWING STEP
    #
    # First, clear the screen to white. Don't put other drawing commands
    # above this, or they will be erased with this command.
    screen.fill(WHITE)
    textPrint.reset()

    # Get count of joysticks.
    joystick_count = pygame.joystick.get_count()

    textPrint.tprint(screen, "Number of joysticks: {}".format(joystick_count))
    textPrint.indent()


    # Get the name from the OS for the controller/joystick.
    name = joystick.get_name()
    textPrint.tprint(screen, "Joystick name: {}".format(name))

    # Usually axis run in pairs, up/down for one, and left/right for
    # the other.
    axes = joystick.get_numaxes()
    textPrint.tprint(screen, "Number of axes: {}".format(axes))
    textPrint.indent()

    for i in range(axes):
        axis = joystick.get_axis(i)
        textPrint.tprint(screen, "Axis {} value: {:>6.3f}".format(i, axis))
    textPrint.unindent()

    buttons = joystick.get_numbuttons()
    textPrint.tprint(screen, "Number of buttons: {}".format(buttons))
    textPrint.indent()

    for i in range(buttons):
        button = joystick.get_button(i)
        textPrint.tprint(screen,
                         "Button {:>2} value: {}".format(i, button))
    textPrint.unindent()

    hats = joystick.get_numhats()
    textPrint.tprint(screen, "Number of hats: {}".format(hats))
    textPrint.indent()

    # Hat position. All or nothing for direction, not a float like
    # get_axis(). Position is a tuple of int values (x, y).
    for i in range(hats):
        hat = joystick.get_hat(i)
        textPrint.tprint(screen, "Hat {} value: {}".format(i, str(hat)))
    textPrint.unindent()


    # byte buttons1;  // 0:Y, 1:B, 2:A, 3:Z, 4:L, 5:R, 6:ZL, 7:ZR
    # byte buttons2;  // 0:-, 1:+, 2:Lstick, 3:Rstick, 4:Home, 5:Capture

    buttons1_mapping = [2, 0, 1, 3, 4, 5]
    buttons1 = 0
    for i, j in enumerate(buttons1_mapping):
        if j == -1:
            continue        
        buttons1 |= joystick.get_button(j) << i
    lr = joystick.get_axis(2)
    if lr >= 0.5:
        buttons1 |= 1 << 6
    elif lr <= -0.5:
        buttons1 |= 1 << 7

    buttons2_mapping = [6, 7, 8, 9, -1, -1]
    buttons2 = 0
    for i, j in enumerate(buttons2_mapping):
        if j == -1:
            continue
        buttons2 |= joystick.get_button(j) << i
    if joystick.get_button(7):
        if lr <= -0.5:
            buttons2 |= 1 << 4
        else:
            buttons2 |= 1 << 1
    if joystick.get_button(6):
        if lr >= 0.5:
            buttons2 |= 1 << 5
        else:
            buttons2 |= 1 << 0

    lx = axis_float_to_byte(joystick.get_axis(0))
    ly = axis_float_to_byte(joystick.get_axis(1))
    rx = axis_float_to_byte(joystick.get_axis(4))
    ry = axis_float_to_byte(joystick.get_axis(3))

    hat_x, hat_y = joystick.get_hat(0)
    hat_idx = (hat_y + 1) * 3 + (hat_x + 1)
    hat_mapping = [5, 4, 3, 6, 8, 2, 7, 0, 1]
    dir_pad = hat_mapping[hat_idx]
    
    textPrint.tprint(screen, f"{buttons1} {buttons2} {lx} {ly} {rx} {ry} {dir_pad}")


    #
    # ALL CODE TO DRAW SHOULD GO ABOVE THIS COMMENT
    #

    # Go ahead and update the screen with what we've drawn.
    pygame.display.flip()


    # Serial stuff
    data = bluetooth_serial.read()
    if len(data) == 1:
        curr_poll_time = pygame.time.get_ticks()
        delay = 0
        avg_poll_delay = 0
        if last_poll_time is not None:
            delay = (curr_poll_time - last_poll_time)
            poll_delays.insert(0, delay)
        if len(poll_delays) > 10:
            poll_delays = poll_delays[:10]
            avg_poll_delay = sum(poll_delays) / len(poll_delays)

        last_poll_time = curr_poll_time
        print(f"Got sync from receiver: avg delay {avg_poll_delay} {delay} {data}")

    bluetooth_serial.write(bytes([0xFF, buttons1, buttons2, lx, ly, rx, ry, dir_pad]))
    #bluetooth_serial.flush()


    # Limit to 20 frames per second.
    clock.tick(75)


# Close the window and quit.
# If you forget this line, the program will 'hang'
# on exit if running from IDLE.
pygame.quit()

bluetooth_serial.close()