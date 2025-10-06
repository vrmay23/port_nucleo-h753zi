/****************************************************************************
 * drivers/lcd/st7796.c - VERSÃO CORRIGIDA
 * 
 * Principais correções:
 * 1. SPI MODE 0 forçado (compatível com ST7796)
 * 2. Timing DC melhorado (15us ao invés de 2us)
 * 3. Sequência de inicialização ajustada conforme fabricante
 * 4. Delay de 120ms após SLPOUT adicionado
 * 5. Valores MADCTL corrigidos
 ****************************************************************************/

#include <nuttx/config.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <debug.h>
#include <nuttx/arch.h>
#include <nuttx/spi/spi.h>
#include <nuttx/video/fb.h>
#include <nuttx/kmalloc.h>
#include <nuttx/clock.h>
#include <nuttx/signal.h>
#include <nuttx/lcd/st7796.h>

#ifdef CONFIG_LCD_ST7796

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* CORREÇÃO 1: Forçar SPI MODE 0 (CPOL=0, CPHA=0) */
#define CONFIG_LCD_ST7796_SPIMODE SPIDEV_MODE0

/* CORREÇÃO 2: Reduzir frequência para 20MHz (mais estável) */
#ifndef CONFIG_LCD_ST7796_FREQUENCY
#  define CONFIG_LCD_ST7796_FREQUENCY 20000000
#endif

/* CORREÇÃO 3: Aumentar timing DC setup/hold */
#define ST7796_DC_SETUP_US    15  /* 15us setup time para DC */
#define ST7796_DC_HOLD_US     15  /* 15us hold time após DC */
#define ST7796_CS_SETUP_US    10  /* 10us CS setup time */

#define ST7796_XRES_RAW    320
#define ST7796_YRES_RAW    480

#if defined(CONFIG_NUCLEO_H753ZI_ST7796_LANDSCAPE) || \
    defined(CONFIG_NUCLEO_H753ZI_ST7796_RLANDSCAPE)
#  define ST7796_XRES       ST7796_YRES_RAW
#  define ST7796_YRES       ST7796_XRES_RAW
#elif defined(CONFIG_LCD_LANDSCAPE) || defined(CONFIG_LCD_RLANDSCAPE)
#  define ST7796_XRES       ST7796_YRES_RAW
#  define ST7796_YRES       ST7796_XRES_RAW
#else
#  define ST7796_XRES       ST7796_XRES_RAW
#  define ST7796_YRES       ST7796_YRES_RAW
#endif

#ifdef CONFIG_LCD_ST7796_BPP
#  if (CONFIG_LCD_ST7796_BPP == 16)
#    define ST7796_BPP           16
#    define ST7796_COLORFMT      FB_FMT_RGB16_565
#    define ST7796_BYTESPP       2
#  elif (CONFIG_LCD_ST7796_BPP == 18)
#    define ST7796_BPP           18
#    define ST7796_COLORFMT      FB_FMT_RGB24
#    define ST7796_BYTESPP       3
#  else
#    define ST7796_BPP           16
#    define ST7796_COLORFMT      FB_FMT_RGB16_565
#    define ST7796_BYTESPP       2
#  endif
#else
#  define ST7796_BPP           16
#  define ST7796_COLORFMT      FB_FMT_RGB16_565
#  define ST7796_BYTESPP       2
#endif

#define ST7796_FBSIZE  (ST7796_XRES * ST7796_YRES * ST7796_BYTESPP)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct st7796_dev_s
{
  struct fb_vtable_s vtable;
  FAR struct spi_dev_s *spi;
  FAR uint8_t *fbmem;
  bool power;
  CODE void (*set_dc)(bool data);
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int st7796_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo);
static int st7796_getplaneinfo(FAR struct fb_vtable_s *vtable, int planeno,
                                FAR struct fb_planeinfo_s *pinfo);
static int st7796_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area);
static void st7796_select(FAR struct spi_dev_s *spi);
static void st7796_deselect(FAR struct spi_dev_s *spi);
static void st7796_sendcmd(FAR struct st7796_dev_s *dev, uint8_t cmd);
static void st7796_senddata(FAR struct st7796_dev_s *dev,
                            FAR const uint8_t *data, size_t len);
static void st7796_send_sequence(FAR struct st7796_dev_s *dev,
                                  FAR const struct st7796_cmd_s *seq,
                                  size_t count);
static void st7796_setarea(FAR struct st7796_dev_s *dev,
                           uint16_t x0, uint16_t y0,
                           uint16_t x1, uint16_t y1);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct st7796_dev_s g_st7796dev;

/* CORREÇÃO 4: MADCTL valores corrigidos conforme datasheet */
static const uint8_t st7796_madctl_data[] = 
{
#if defined(CONFIG_NUCLEO_H753ZI_ST7796_LANDSCAPE)
  0x28  /* Landscape: MV=1, BGR=1 */
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RPORTRAIT)
  0x88  /* Reverse Portrait: MY=1, BGR=1 */
#elif defined(CONFIG_NUCLEO_H753ZI_ST7796_RLANDSCAPE)
  0xE8  /* Reverse Landscape: MY=1, MX=1, MV=1, BGR=1 */
#else
  0x48  /* Portrait: MX=1, BGR=1 (padrão do fabricante) */
#endif
};

/* CORREÇÃO 5: Sequência conforme código do fabricante */
static const struct st7796_cmd_s st7796_init_sequence[] =
{
  /* Sleep Out - CRÍTICO: precisa de 120ms delay */
  {0x11, NULL, 0, 120},
  
  /* Command Set Control (CSCON) Enable - sequência exata do fabricante */
  {0xF0, (const uint8_t[]){0xC3}, 1, 0},
  {0xF0, (const uint8_t[]){0x96}, 1, 0},
  
  /* Memory Access Control */
  {0x36, st7796_madctl_data, 1, 0},
  
  /* Interface Pixel Format: 16-bit/pixel (RGB565) */
  {0x3A, (const uint8_t[]){0x55}, 1, 0},
  
  /* Display Inversion Control */
  {0xB4, (const uint8_t[]){0x01}, 1, 0},
  
  /* Display Function Control */
  {0xB7, (const uint8_t[]){0xC6}, 1, 0},
  
  /* Display Output Ctrl Adjust */
  {0xE8, (const uint8_t[]){0x40, 0x8A, 0x00, 0x00, 
                           0x29, 0x19, 0xA5, 0x33}, 8, 0},
  
  /* Power Control 2 */
  {0xC1, (const uint8_t[]){0x06}, 1, 0},
  
  /* Power Control 3 */
  {0xC2, (const uint8_t[]){0xA7}, 1, 0},
  
  /* VCOM Control */
  {0xC5, (const uint8_t[]){0x18}, 1, 0},
  
  /* Positive Gamma Correction */
  {0xE0, (const uint8_t[]){0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F,
                           0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B}, 14, 0},
  
  /* Negative Gamma Correction */
  {0xE1, (const uint8_t[]){0xF0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2D,
                           0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B}, 14, 0},
  
  /* Command Set Control Disable */
  {0xF0, (const uint8_t[]){0x3C}, 1, 0},
  {0xF0, (const uint8_t[]){0x69}, 1, 0},
  
  /* Display Inversion On */
  {0x21, NULL, 0, 0},
  
  /* Display On - CRÍTICO: precisa de 120ms delay */
  {0x29, NULL, 0, 120},
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void st7796_select(FAR struct spi_dev_s *spi)
{
  SPI_LOCK(spi, true);
  
  /* CORREÇÃO 6: Garantir que MODE0 está configurado */
  SPI_SETMODE(spi, SPIDEV_MODE0);
  SPI_SETBITS(spi, 8);
  SPI_SETFREQUENCY(spi, CONFIG_LCD_ST7796_FREQUENCY);
  
  /* CS setup time */
  up_udelay(ST7796_CS_SETUP_US);
  
  SPI_SELECT(spi, SPIDEV_DISPLAY(0), true);
  
  /* Aguardar estabilização CS */
  up_udelay(ST7796_CS_SETUP_US);
}

static void st7796_deselect(FAR struct spi_dev_s *spi)
{
  /* CS hold time antes de desativar */
  up_udelay(ST7796_CS_SETUP_US);
  
  SPI_SELECT(spi, SPIDEV_DISPLAY(0), false);
  SPI_LOCK(spi, false);
}

/* CORREÇÃO 7: Timing DC melhorado */
static void st7796_sendcmd(FAR struct st7796_dev_s *dev, uint8_t cmd)
{
  if (dev->set_dc)
    {
      dev->set_dc(false);  /* DC = 0 para comando */
    }

  /* CRÍTICO: Aumentar DC setup time de 2us para 15us */
  up_udelay(ST7796_DC_SETUP_US);
  
  SPI_SEND(dev->spi, cmd);
  
  /* CRÍTICO: Aguardar comando ser processado antes de mudar DC */
  up_udelay(ST7796_DC_HOLD_US);
}

/* Teste de diagnóstico: enviar comando e ler resposta */
static uint8_t st7796_read_register(FAR struct st7796_dev_s *dev, uint8_t reg)
{
  uint8_t result = 0;
  
  st7796_select(dev->spi);
  st7796_sendcmd(dev, reg);
  
  if (dev->set_dc)
    {
      dev->set_dc(true);
    }
  up_udelay(ST7796_DC_SETUP_US);
  
  SPI_RECVBLOCK(dev->spi, &result, 1);
  st7796_deselect(dev->spi);
  
  return result;
}

static void st7796_senddata(FAR struct st7796_dev_s *dev,
                            FAR const uint8_t *data, size_t len)
{
  if (len > 0 && data != NULL)
    {
      if (dev->set_dc)
        {
          dev->set_dc(true);  /* DC = 1 para dados */
        }

      /* CRÍTICO: DC setup time */
      up_udelay(ST7796_DC_SETUP_US);
      
      SPI_SNDBLOCK(dev->spi, data, len);
      
      /* CRÍTICO: DC hold time */
      up_udelay(ST7796_DC_HOLD_US);
    }
}

/****************************************************************************
 * CORREÇÃO CRÍTICA DE TIMING
 * 
 * Problema: Display intermitente (às vezes backlight, às vezes zebrado)
 * Causa: Timing entre comandos muito apertado + sem DMA
 * 
 * Solução: Adicionar delays entre TODOS os comandos
 ****************************************************************************/

/* Substitua a função st7796_send_sequence por esta versão: */

static void st7796_send_sequence(FAR struct st7796_dev_s *dev,
                                  FAR const struct st7796_cmd_s *seq,
                                  size_t count)
{
  size_t i;

  syslog(LOG_INFO, "ST7796: Iniciando sequência de %zu comandos\n", count);
  syslog(LOG_INFO, "ST7796: SPI MODE=%d, Freq=%d Hz\n", 
         SPIDEV_MODE0, CONFIG_LCD_ST7796_FREQUENCY);

  for (i = 0; i < count; i++)
    {
      syslog(LOG_INFO, "ST7796: CMD[%zu]=0x%02X len=%zu delay=%ums\n",
             i, seq[i].cmd, seq[i].len, seq[i].delay_ms);

      /* CRÍTICO: Select CS */
      st7796_select(dev->spi);
      
      /* CRÍTICO: Delay adicional antes de enviar comando */
      up_udelay(50);  /* 50us de estabilização */
      
      /* Enviar comando */
      st7796_sendcmd(dev, seq[i].cmd);
      
      /* CRÍTICO: Delay entre comando e dados */
      if (seq[i].data != NULL && seq[i].len > 0)
        {
          up_udelay(50);  /* 50us entre comando e dados */
          st7796_senddata(dev, seq[i].data, seq[i].len);
        }
      
      /* CRÍTICO: Delay antes de desativar CS */
      up_udelay(50);
      
      /* Deselect CS */
      st7796_deselect(dev->spi);
      
      /* CRÍTICO: Delay MÍNIMO entre comandos diferentes */
      up_udelay(100);  /* 100us entre comandos */
      
      /* Delay programado do comando */
      if (seq[i].delay_ms > 0)
        {
          syslog(LOG_INFO, "ST7796: Aguardando %ums...\n", seq[i].delay_ms);
          nxsig_usleep(seq[i].delay_ms * 1000);
        }
    }

  syslog(LOG_INFO, "ST7796: Inicialização concluída com sucesso\n");
}

/****************************************************************************
 * EXPLICAÇÃO DOS DELAYS ADICIONADOS
 * 
 * 1. 50us antes do comando: Garante CS estável
 * 2. 50us entre CMD e DATA: Display precisa processar comando
 * 3. 50us antes de desativar CS: Garante última transmissão completa
 * 4. 100us entre comandos: Display processa e fica pronto para próximo
 * 
 * Com SPI a 20MHz sem DMA, esses delays são CRÍTICOS para confiabilidade.
 ****************************************************************************/

static void st7796_setarea(FAR struct st7796_dev_s *dev,
                           uint16_t x0, uint16_t y0,
                           uint16_t x1, uint16_t y1)
{
  uint8_t data[4];

  /* Column Address Set */
  st7796_sendcmd(dev, 0x2A);
  data[0] = (x0 >> 8) & 0xFF;
  data[1] = x0 & 0xFF;
  data[2] = (x1 >> 8) & 0xFF;
  data[3] = x1 & 0xFF;
  st7796_senddata(dev, data, 4);

  /* Row Address Set */
  st7796_sendcmd(dev, 0x2B);
  data[0] = (y0 >> 8) & 0xFF;
  data[1] = y0 & 0xFF;
  data[2] = (y1 >> 8) & 0xFF;
  data[3] = y1 & 0xFF;
  st7796_senddata(dev, data, 4);
}

static int st7796_getvideoinfo(FAR struct fb_vtable_s *vtable,
                                FAR struct fb_videoinfo_s *vinfo)
{
  DEBUGASSERT(vtable && vinfo);

  vinfo->fmt     = ST7796_COLORFMT;
  vinfo->xres    = ST7796_XRES;
  vinfo->yres    = ST7796_YRES;
  vinfo->nplanes = 1;

  return OK;
}

static int st7796_getplaneinfo(FAR struct fb_vtable_s *vtable, int planeno,
                                FAR struct fb_planeinfo_s *pinfo)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;

  DEBUGASSERT(vtable && pinfo && planeno == 0);

  pinfo->fbmem   = priv->fbmem;
  pinfo->fblen   = ST7796_FBSIZE;
  pinfo->stride  = ST7796_XRES * ST7796_BYTESPP;
  pinfo->bpp     = ST7796_BPP;
  pinfo->xres_virtual = ST7796_XRES;
  pinfo->yres_virtual = ST7796_YRES;
  pinfo->xoffset = 0;
  pinfo->yoffset = 0;

  return OK;
}

static int st7796_updatearea(FAR struct fb_vtable_s *vtable,
                              FAR const struct fb_area_s *area)
{
  FAR struct st7796_dev_s *priv = (FAR struct st7796_dev_s *)vtable;
  FAR uint8_t *fbptr;
  size_t row_size;
  int row;

  st7796_select(priv->spi);

  st7796_setarea(priv, area->x, area->y,
                 area->x + area->w - 1,
                 area->y + area->h - 1);

  st7796_sendcmd(priv, 0x2C);  /* Memory Write */

  if (priv->set_dc)
    {
      priv->set_dc(true);
    }

  up_udelay(ST7796_DC_SETUP_US);

  row_size = area->w * ST7796_BYTESPP;
  fbptr = priv->fbmem + (area->y * ST7796_XRES + area->x) * ST7796_BYTESPP;

  for (row = 0; row < area->h; row++)
    {
      SPI_SNDBLOCK(priv->spi, fbptr, row_size);
      fbptr += ST7796_XRES * ST7796_BYTESPP;
    }

  st7796_deselect(priv->spi);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

FAR struct fb_vtable_s *st7796_fbinitialize(FAR struct spi_dev_s *spi,
                                            CODE void (*set_dc)(bool))
{
  FAR struct st7796_dev_s *priv = &g_st7796dev;

  syslog(LOG_INFO, "ST7796: Inicializando driver framebuffer\n");
  syslog(LOG_INFO, "ST7796: Resolução: %dx%d @ %d bpp\n",
         ST7796_XRES, ST7796_YRES, ST7796_BPP);
  syslog(LOG_INFO, "ST7796: Tamanho FB: %d KB\n", ST7796_FBSIZE / 1024);

  /* Alocar framebuffer */
  priv->fbmem = (FAR uint8_t *)kmm_zalloc(ST7796_FBSIZE);
  if (!priv->fbmem)
    {
      syslog(LOG_ERR, "ERRO: Falha ao alocar framebuffer\n");
      return NULL;
    }

  syslog(LOG_INFO, "ST7796: Framebuffer alocado com sucesso\n");

  /* Inicializar estrutura */
  priv->vtable.getvideoinfo  = st7796_getvideoinfo;
  priv->vtable.getplaneinfo  = st7796_getplaneinfo;
  priv->vtable.updatearea    = st7796_updatearea;
  priv->spi                  = spi;
  priv->power                = false;
  priv->set_dc               = set_dc;

  /* Executar sequência de inicialização */
  st7796_send_sequence(priv, st7796_init_sequence,
                       sizeof(st7796_init_sequence) /
                       sizeof(struct st7796_cmd_s));

  priv->power = true;

  syslog(LOG_INFO, "ST7796: Display pronto para uso\n");

  /* TESTE DIAGNÓSTICO APRIMORADO: Ler ID do display */
  uint8_t dummy, id1, id2, id3;
  uint8_t test_byte;
  
  syslog(LOG_INFO, "ST7796: === TESTE DE COMUNICAÇÃO SPI ===\n");
  
  /* Teste 1: Verificar se consegue ler algo */
  st7796_select(priv->spi);
  test_byte = SPI_SEND(priv->spi, 0xFF);
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: Teste loopback: enviou 0xFF, recebeu 0x%02X\n", test_byte);
  
  /* Teste 2: Tentar ler ID com mais dummy bytes */
  st7796_select(priv->spi);
  st7796_sendcmd(priv, 0x04);  /* Read Display ID */
  
  if (priv->set_dc)
    {
      priv->set_dc(true);
    }
  up_udelay(50);  /* Aumentar delay para 50us */
  
  /* Alguns ST7796 precisam de 2+ dummy bytes */
  SPI_RECVBLOCK(priv->spi, &dummy, 1);  /* Dummy 1 */
  syslog(LOG_INFO, "ST7796: Dummy byte 1 = 0x%02X\n", dummy);
  
  SPI_RECVBLOCK(priv->spi, &dummy, 1);  /* Dummy 2 */
  syslog(LOG_INFO, "ST7796: Dummy byte 2 = 0x%02X\n", dummy);
  
  SPI_RECVBLOCK(priv->spi, &id1, 1);  /* ID1 */
  SPI_RECVBLOCK(priv->spi, &id2, 1);  /* ID2 */
  SPI_RECVBLOCK(priv->spi, &id3, 1);  /* ID3 */
  st7796_deselect(priv->spi);
  
  syslog(LOG_INFO, "ST7796: Display ID = 0x%02X 0x%02X 0x%02X\n", id1, id2, id3);
  syslog(LOG_INFO, "ST7796: Esperado: 0x00 0x77 0x96 (ST7796)\n");
  
  if (id1 == 0x00 && id2 == 0x00 && id3 == 0x00)
    {
      syslog(LOG_WARNING, "ST7796: AVISO - ID retornou zeros!\n");
      syslog(LOG_WARNING, "ST7796: Possíveis causas:\n");
      syslog(LOG_WARNING, "ST7796:   1. MISO não conectado ou com mau contato\n");
      syslog(LOG_WARNING, "ST7796:   2. Display em modo de proteção\n");
      syslog(LOG_WARNING, "ST7796:   3. Comando de leitura não suportado neste lote\n");
      syslog(LOG_WARNING, "ST7796: Tentando continuar mesmo assim...\n");
    }
  else if (id1 == 0xFF && id2 == 0xFF && id3 == 0xFF)
    {
      syslog(LOG_ERR, "ST7796: ERRO - ID retornou 0xFF (bus flutuante)!\n");
      syslog(LOG_ERR, "ST7796: Display provavelmente não está respondendo.\n");
    }
  else if (id2 == 0x77 && id3 == 0x96)
    {
      syslog(LOG_INFO, "ST7796: ID correto detectado!\n");
    }
  
  syslog(LOG_INFO, "ST7796: === FIM DO TESTE ===\n");
  
  /* TESTE ADICIONAL: Tentar escrever pixels diretamente */
  syslog(LOG_INFO, "ST7796: Iniciando teste de escrita de pixels...\n");
  
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, 99, 0);  /* Linha de 100 pixels */
  st7796_sendcmd(priv, 0x2C);  /* Memory Write */
  
  if (priv->set_dc)
    {
      priv->set_dc(true);
    }
  up_udelay(ST7796_DC_SETUP_US);
  
  /* Enviar 100 pixels vermelhos (RGB565 = 0xF800) */
  uint8_t red_pixel[2] = {0xF8, 0x00};
  for (int i = 0; i < 100; i++)
    {
      SPI_SNDBLOCK(priv->spi, red_pixel, 2);
    }
  
  st7796_deselect(priv->spi);
  
  syslog(LOG_INFO, "ST7796: Teste de pixels concluído (100 pixels vermelhos no topo)\n");
  syslog(LOG_INFO, "ST7796: Se ver pixels vermelhos = escrita SPI OK\n");
  syslog(LOG_INFO, "ST7796: Se continuar zebrado = problema na inicialização\n");
  
  /* TESTE DE CORES COMPLETO */
  syslog(LOG_INFO, "ST7796: === INICIANDO TESTE DE CORES ===\n");
  
  /* Aguardar estabilização */
  nxsig_usleep(500000);  /* 500ms */
  
  /* RED - Tela inteira vermelha */
  syslog(LOG_INFO, "ST7796: Preenchendo tela com VERMELHO...\n");
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, ST7796_XRES - 1, ST7796_YRES - 1);
  st7796_sendcmd(priv, 0x2C);
  if (priv->set_dc) priv->set_dc(true);
  up_udelay(ST7796_DC_SETUP_US);
  
  uint8_t red[2] = {0xF8, 0x00};  /* RGB565: 11111 000000 00000 */
  for (uint32_t i = 0; i < (ST7796_XRES * ST7796_YRES); i++)
    {
      SPI_SNDBLOCK(priv->spi, red, 2);
    }
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: VERMELHO completo\n");
  nxsig_usleep(2000000);  /* 2 segundos */
  
  /* GREEN - Tela inteira verde */
  syslog(LOG_INFO, "ST7796: Preenchendo tela com VERDE...\n");
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, ST7796_XRES - 1, ST7796_YRES - 1);
  st7796_sendcmd(priv, 0x2C);
  if (priv->set_dc) priv->set_dc(true);
  up_udelay(ST7796_DC_SETUP_US);
  
  uint8_t green[2] = {0x07, 0xE0};  /* RGB565: 00000 111111 00000 */
  for (uint32_t i = 0; i < (ST7796_XRES * ST7796_YRES); i++)
    {
      SPI_SNDBLOCK(priv->spi, green, 2);
    }
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: VERDE completo\n");
  nxsig_usleep(2000000);
  
  /* BLUE - Tela inteira azul */
  syslog(LOG_INFO, "ST7796: Preenchendo tela com AZUL...\n");
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, ST7796_XRES - 1, ST7796_YRES - 1);
  st7796_sendcmd(priv, 0x2C);
  if (priv->set_dc) priv->set_dc(true);
  up_udelay(ST7796_DC_SETUP_US);
  
  uint8_t blue[2] = {0x00, 0x1F};  /* RGB565: 00000 000000 11111 */
  for (uint32_t i = 0; i < (ST7796_XRES * ST7796_YRES); i++)
    {
      SPI_SNDBLOCK(priv->spi, blue, 2);
    }
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: AZUL completo\n");
  nxsig_usleep(2000000);
  
  /* WHITE - Tela inteira branca */
  syslog(LOG_INFO, "ST7796: Preenchendo tela com BRANCO...\n");
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, ST7796_XRES - 1, ST7796_YRES - 1);
  st7796_sendcmd(priv, 0x2C);
  if (priv->set_dc) priv->set_dc(true);
  up_udelay(ST7796_DC_SETUP_US);
  
  uint8_t white[2] = {0xFF, 0xFF};  /* RGB565: 11111 111111 11111 */
  for (uint32_t i = 0; i < (ST7796_XRES * ST7796_YRES); i++)
    {
      SPI_SNDBLOCK(priv->spi, white, 2);
    }
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: BRANCO completo\n");
  nxsig_usleep(2000000);
  
  /* BLACK - Tela inteira preta */
  syslog(LOG_INFO, "ST7796: Preenchendo tela com PRETO...\n");
  st7796_select(priv->spi);
  st7796_setarea(priv, 0, 0, ST7796_XRES - 1, ST7796_YRES - 1);
  st7796_sendcmd(priv, 0x2C);
  if (priv->set_dc) priv->set_dc(true);
  up_udelay(ST7796_DC_SETUP_US);
  
  uint8_t black[2] = {0x00, 0x00};  /* RGB565: 00000 000000 00000 */
  for (uint32_t i = 0; i < (ST7796_XRES * ST7796_YRES); i++)
    {
      SPI_SNDBLOCK(priv->spi, black, 2);
    }
  st7796_deselect(priv->spi);
  syslog(LOG_INFO, "ST7796: PRETO completo\n");
  
  syslog(LOG_INFO, "ST7796: === TESTE DE CORES CONCLUÍDO ===\n");
  syslog(LOG_INFO, "ST7796: Verifique se viu: VERMELHO -> VERDE -> AZUL -> BRANCO -> PRETO\n");
  syslog(LOG_INFO, "ST7796: Se cores apareceram = display funcionando!\n");
  syslog(LOG_INFO, "ST7796: Se ficou zebrado = problema na inicialização\n");

  return &priv->vtable;
}

#endif /* CONFIG_LCD_ST7796 */
