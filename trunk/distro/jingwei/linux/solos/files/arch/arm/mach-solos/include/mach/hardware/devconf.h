/*
 * linux/include/asm-arm/arch-solos/hardware/devconf.h
 * Copyright (C) 2005 Conexant
 */

/* Device configuration registers on the lower AHB */
#ifndef __ASM_ARCH_DEVCONF
#define __ASM_ARCH_DEVCONF


#define SOLOS_DEVCONF_CONTROL		0x00
#define SOLOS_DEVCONF_CLOCK_DIV		0x04
#define SOLOS_DEVCONF_BOOT_CONTROL	0x08
#define SOLOS_DEVCONF_RESET_CONTROL 	0x0C
#define SOLOS_DEVCONF_8M_CLK_CONTROL 	0x10
#define SOLOS_DEVCONF_PLL_SELECT 	0x14
#define SOLOS_DEVCONF_PLL_WRITE_DATA 	0x18
#define SOLOS_DEVCONF_PLL_REG_SEL 	0x1C
#define SOLOS_DEVCONF_PLL_CONTROL 	0x20
#define SOLOS_DEVCONF_PLL_READ_DATA 	0x24
#define SOLOS_DEVCONF_USB2_ISI_PHY 	0x28
#define SOLOS_DEVCONF_CLK_ENABLE 	0x2C
#define SOLOS_DEVCONF_CONFIG_DIAGNOSTIC 0x74

#define SOLOS_DEVCONF_CONFIG_STUN_ULPI_MUX_EN   (0x40000000)
#define SOLOS_DEVCONF_DIAGNOSTIC_USB11_ENABLE   (0x00400000)
#define SOLOS_DEVCONF_CONFIG_EXT_USB_PHY_EN    (0x00200000)

#define VIEWPORT_ADDR (SOLOS_UAHB_VIRT + SOLOS_USB_CAPOFFSET + EHCI_ULPI_VIEWPORT + 0x40)

/* CONFIG VERSION NUMBER */
/* INDICATES SILICON REV */

#define SOLOS_DEVCONF_CONTROL_VER_NUM_MASK     (0x0000ffff)

#define SOLOS_DEVCONF_CONTROL_EXT_DSP_CLK_MODE (0x00100000)
#define SOLOS_DEVCONF_CONTROL_USB0_CLK_DISABLE (0x00080000)
#define SOLOS_DEVCONF_CONTROL_USB1_CLK_DISABLE (0x00040000)
#define SOLOS_DEVCONF_CONTROL_MAC0_CLK_DISABLE (0x00020000)
#define SOLOS_DEVCONF_CONTROL_MAC1_CLK_DISABLE (0x00010000)

// CONFIG_CLOCK_DIV

#define SOLOS_DEVCONF_CLOCK_DIV_MAIN_MASK  (0x000000F0)
#define SOLOS_DEVCONF_CLOCK_DIV_MAIN_SHIFT (4)
#define SOLOS_DEVCONF_CLOCK_DIV_8MHZ_MASK  (0x0000000F)
#define SOLOS_DEVCONF_CLOCK_DIV_8MHZ_SHIFT (0)

// CONFIG_BOOT_CONTROL
#define SOLOS_DEVCONF_MISSION_MODE         (0x00000001)
#define SOLOS_DEVCONF_RTL_SIM_MODE         (0x00000002)
#define SOLOS_DEVCONF_OUT_OF_RESET         (0x00000010)

// CONFIG_RESET_CONTROL
#define SOLOS_DEVCONF_RESET_PRE5CONF_MASK  (0xFF000000)
#define SOLOS_DEVCONF_RESET_PRE5CONF_SHIFT (24)
#define SOLOS_DEVCONF_TRIGGER_N_RESET_OUT  (0x00000001)

// CONFIG_8M_CLK_CONTROL
#define SOLOS_DEVCONF_8MCLK_XVAL_MASK      (0x0000FF00)
#define SOLOS_DEVCONF_8MCLK_XVAL_SHIFT     (8)
#define SOLOS_DEVCONF_8MCLK_YVAL_MASK      (0x000000FF)
#define SOLOS_DEVCONF_8MCLK_YVAL_SHIFT     (0)
#define SOLOS_DEVCONF_8MCLK_REF_NORM       (0x00000000)
#define SOLOS_DEVCONF_8MCLK_REF_FAST       (0x00010000)

// PLL Setting values
#define SOLOS_DEVCONF_SEL_SYS_PLL          (0x00000001)
#define SOLOS_DEVCONF_SEL_NET_PLL          (0x00000002)
#define SOLOS_DEVCONF_SEL_AFE_PLL          (0x00000004)
#define SOLOS_DEVCONF_SEL_DSP_PLL          (0x00000008)

// SYS_PLL 1Ghz PLL clock
#define SOLOS_DEVCONF_SYS_PLL_N_VAL        ((20<<24)+(140<<12)+(243)) // 20+(140/243): 51.84->1066.6667 -> 266.666

// NET_PLL 1.2Ghz PLL clock
#define SOLOS_DEVCONF_NET_PLL_N_VAL        ((23<<24)+(  4<<12)+( 27)) // 23+(  4/ 27): 51.84->12000     -> 300.00
#define SOLOS_DEVCONF_AFE_PLL_N_VAL        ((2 <<24)+( 98<<12)+(135)) // 2 +( 98/135): 51.84->141.312   -> 35.328
#define SOLOS_DEVCONF_DSP_PLL_N_VAL        ((19<<24)+( 47<<12)+(162)) // 19+( 47/162): 51.84->10000     -> 250.00

#define SOLOS_DEVCONF_SEL_REG_2            (0x00000004)

// CONFIG_PLL_CONTROL
#define SOLOS_DEVCONF_PLL_CONTROL_HIREQ    (0x00000001)
#define SOLOS_DEVCONF_PLL_CONTROL_HIW_RN   (0x00000002)

#define SOLOS_DEVCONF_PLL_WRITE_SETUP      (0x00000003)
#define SOLOS_DEVCONF_PLL_WRITE            (0x00000002)
#define SOLOS_DEVCONF_PLL_READ             (0x00000001)
#define SOLOS_DEVCONF_PLL_WRITE_STOP       (0x00000000)

#define SOLOS_DEVCONF_PLL_DIV_1            (0x00000000)
#define SOLOS_DEVCONF_PLL_DIV_2            (0x00001000)
#define SOLOS_DEVCONF_PLL_DIV_3            (0x00002000)
#define SOLOS_DEVCONF_PLL_DIV_4            (0x00003000)
#define SOLOS_DEVCONF_PLL_DIV_5            (0x00004000)
#define SOLOS_DEVCONF_PLL_DIV_6            (0x00005000)
#define SOLOS_DEVCONF_PLL_DIV_7            (0x00006000)
#define SOLOS_DEVCONF_PLL_DIV_8            (0x00007000)

#define SOLOS_DEVCONF_PLL_DIV_4_XLOL_2     (0x02003000)
#define SOLOS_DEVCONF_PLL_DIV_4_XLOL_7     (0x07003000)

#define SOLOS_DEVCONF_PLL_XLOL_0           (0x00000000)
#define SOLOS_DEVCONF_PLL_XLOL_1           (0x01000000)
#define SOLOS_DEVCONF_PLL_XLOL_2           (0x02000000)
#define SOLOS_DEVCONF_PLL_XLOL_3           (0x03000000)
#define SOLOS_DEVCONF_PLL_XLOL_4           (0x04000000)
#define SOLOS_DEVCONF_PLL_XLOL_5           (0x05000000)
#define SOLOS_DEVCONF_PLL_XLOL_6           (0x06000000)
#define SOLOS_DEVCONF_PLL_XLOL_7           (0x07000000)
#define SOLOS_DEVCONF_SEL_REG_0            (0x00000001)
#define SOLOS_DEVCONF_SEL_REG_1            (0x00000002)
#define SOLOS_DEVCONF_SEL_REG_2            (0x00000004)

#define SOLOS_DEVCONF_SEL_REG_3            (0x00000008)
#define SOLOS_DEVCONF_SEL_REG_4            (0x00000010)
#define SOLOS_DEVCONF_SEL_REG_5            (0x00000020)
#define SOLOS_DEVCONF_SEL_REG_6            (0x00000040)
#define SOLOS_DEVCONF_SEL_REG_7            (0x00000080)

#define SOLOS_DEVCONF_SEL_REG_8            (0x00000100)
#define SOLOS_DEVCONF_SEL_REG_9            (0x00000200)

#define SOLOS_DEVCONF_CLK_ENABLE_SYS_NRST  (0x00000001) /* Reset control for SYS PLL    */
#define SOLOS_DEVCONF_CLK_ENABLE_NET_NRST  (0x00000002) /* Reset control for NET PLL    */
#define SOLOS_DEVCONF_CLK_ENABLE_AFE_NRST  (0x00000004) /* Reset control for AFE PL  L  */
#define SOLOS_DEVCONF_CLK_ENABLE_DSP_NRST  (0x00000008) /* Reset control for DSP   PLL  */
#define SOLOS_DEVCONF_CLK_ENABLE_SYS_EN    (0x00000010) /* Enable the System PLL clock  */
#define SOLOS_DEVCONF_CLK_ENABLE_NET_EN    (0x00000020) /* Enable the Network PLL clock */
#define SOLOS_DEVCONF_CLK_ENABLE_DSP_EN    (0x00000040) /* Enable the DSL DSP PLL clock */
#define SOLOS_DEVCONF_CLK_ENABLE_AFE_EN    (0x00000080) /* Enable the AFE PLL clock     */
#define SOLOS_DEVCONF_CLK_ENABLE_DLL_EN    (0x00000100) /* Enable the DLL PLL clock     */
#define SOLOS_DEVCONF_CLK_ENABLE_SYS_LOCK  (0x00010000) /* The System PLL is locked     */
#define SOLOS_DEVCONF_CLK_ENABLE_NET_LOCK  (0x00020000) /* The Network PLL is locked    */
#define SOLOS_DEVCONF_CLK_ENABLE_DSP_LOCK  (0x00040000) /* The DSP PLL is locked        */
#define SOLOS_DEVCONF_CLK_ENABLE_AFE_LOCK  (0x00080000) /* The AFE PLL is locked        */

// ISI USB2.0 PHY

#define SOLOS_DEVCONF_USB_PHY0_SUSPEND_EN  (0x00000001)
#define SOLOS_DEVCONF_USB_PHY0_WAKEUP_EN   (0x00000002)
#define SOLOS_DEVCONF_USB_PHY0_TXBS_ENH    (0x00000004)
#define SOLOS_DEVCONF_USB_PHY0_TXBS_EN     (0x00000008)
#define SOLOS_DEVCONF_USB_PHY0_CLK12SEL    (0x00000010)
#define SOLOS_DEVCONF_USB_PHY0_KEEP12MHZ   (0x00000020)
#define SOLOS_DEVCONF_USB_PHY0_KEEPOTG     (0x00000040)
#define SOLOS_DEVCONF_USB_PHY0_CLK_RESET   (0x00000080)
#define SOLOS_DEVCONF_USB_PHY0_ULPICLK_EN  (0x00000100)
#define SOLOS_DEVCONF_USB_PHY0_CLK_VALID   (0x00000200) /* PHY0 PLL Clock Output is Valid */

#define SOLOS_DEVCONF_USB_PHY1_SUSPEND_EN  (0x00010000)
#define SOLOS_DEVCONF_USB_PHY1_WAKEUP_EN   (0x00020000)
#define SOLOS_DEVCONF_USB_PHY1_TXBS_ENH    (0x00040000)
#define SOLOS_DEVCONF_USB_PHY1_TXBS_EN     (0x00080000)
#define SOLOS_DEVCONF_USB_PHY1_CLK12SEL    (0x00100000)
#define SOLOS_DEVCONF_USB_PHY1_KEEP12MHZ   (0x00200000)
#define SOLOS_DEVCONF_USB_PHY1_KEEPOTG     (0x00400000)
#define SOLOS_DEVCONF_USB_PHY1_CLK_RESET   (0x00800000)
#define SOLOS_DEVCONF_USB_PHY1_CLK_VALID   (0x01000000) /* PHY1 PLL Clock Output is Valid */


#endif
