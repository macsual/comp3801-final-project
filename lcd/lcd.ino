#define LCD_DATA_PORT   PORTB
#define LCD_DATA_DDR    DDRB
#define LCD_DATA_PIN    PINB

#define LCD_CNTRL_PORT  PORTD
#define LCD_CNTRL_DDR   DDRD
#define LCD_CNTRL_PIN   PIND

#define LCD_RS_PIN      4
#define LCD_RW_PIN      5
#define LCD_ENABLE_PIN  6

static void lcd_send_command(unsigned char cmnd);
static void lcd_send_data(unsigned char data);
static void lcd_init(void);
static void lcd_goto(unsigned char y, unsigned char x);
static void lcd_print(const char *s);

static void 
lcd_init(void)
{
    LCD_CNTRL_DDR = 0xFF;
    LCD_CNTRL_PORT = 0x00;
    LCD_DATA_DDR = 0xFF;
    LCD_DATA_PORT = 0x00;
    
    delay(10);
    lcd_send_command(0x38);
    lcd_send_command(0x0C);
    lcd_send_command(0x01);
    delay(10);
    lcd_send_command(0x06);
}

/* This function sends a command 'cmnd' to the LCD module*/
static void 
lcd_send_command(unsigned char cmnd)
{
    LCD_DATA_PORT = cmnd;
    LCD_CNTRL_PORT &= ~(1u << LCD_RW_PIN);
    LCD_CNTRL_PORT &= ~(1u << LCD_RS_PIN);
    
    LCD_CNTRL_PORT |= (1u << LCD_ENABLE_PIN);
    delayMicroseconds(2);
    LCD_CNTRL_PORT &= ~(1u << LCD_ENABLE_PIN);
    delayMicroseconds(100);
}

/* This function moves the cursor the line y column x on the LCD module*/
static void 
lcd_goto(unsigned char y, unsigned char x)
{
    const unsigned char firstAddress[] = {0x80, 0xC0, 0x94, 0xD4};
    
    lcd_send_command(firstAddress[y - 1] + x - 1);
    delay(10);
}

/* This function sends the data 'data' to the LCD module*/
static void 
lcd_send_data(unsigned char data)
{
    LCD_DATA_PORT = data;             
    LCD_CNTRL_PORT &= ~(1u << LCD_RW_PIN);
    LCD_CNTRL_PORT |= (1u << LCD_RS_PIN);
    
    LCD_CNTRL_PORT |= (1u << LCD_ENABLE_PIN);
    delayMicroseconds(2);
    LCD_CNTRL_PORT &= ~(1u << LCD_ENABLE_PIN);
    delayMicroseconds(100);
}

static void 
lcd_print(const char *s)
{   
    while(*s) {
        lcd_send_data(*s);
        s++;
    }
}

void
setup(void)
{
  lcd_init();
  lcd_goto(1, 1);
  lcd_print("Strongbox");
}

void
loop(void)
{
  /* empty */
}
