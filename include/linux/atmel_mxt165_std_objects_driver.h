#ifndef STD_OBJECTS_H
#define STD_OBJECTS_H

/*! ===Header File Version Number=== */
#define OBJECT_LIST__VERSION_NUMBER	          0x10

#define RESERVED_T0                               0u
#define RESERVED_T1                               1u
#define DEBUG_DELTAS_T2                           2u
#define DEBUG_REFERENCES_T3                       3u
#define DEBUG_SIGNALS_T4                          4u
#define GEN_MESSAGEPROCESSOR_T5                   5u
#define GEN_COMMANDPROCESSOR_T6                   6u
#define GEN_POWERCONFIG_T7                        7u
#define GEN_ACQUISITIONCONFIG_T8                  8u
#define TOUCH_MULTITOUCHSCREEN_T9                 9u
#define TOUCH_SINGLETOUCHSCREEN_T10               10u
#define TOUCH_XSLIDER_T11                         11u
#define TOUCH_YSLIDER_T12                         12u
#define TOUCH_XWHEEL_T13                          13u
#define TOUCH_YWHEEL_T14                          14u
#define TOUCH_KEYARRAY_T15                        15u
#define PROCG_SIGNALFILTER_T16                    16u
#define PROCI_LINEARIZATIONTABLE_T17              17u
#define SPT_COMCONFIG_T18                         18u
#define SPT_GPIOPWM_T19                           19u
#define PROCI_GRIPFACESUPPRESSION_T20             20u
#define RESERVED_T21                              21u
#define PROCG_NOISESUPPRESSION_T22                22u
#define TOUCH_PROXIMITY_T23	                      23u
#define PROCI_ONETOUCHGESTUREPROCESSOR_T24        24u
#define SPT_SELFTEST_T25                          25u
#define DEBUG_CTERANGE_T26                        26u
#define PROCI_TWOTOUCHGESTUREPROCESSOR_T27        27u
#define SPT_CTECONFIG_T28                         28u
#define SPT_GPI_T29                               29u
#define SPT_GATE_T30                              30u
#define TOUCH_KEYSET_T31                          31u
#define TOUCH_XSLIDERSET_T32                      32u
#define SPARE_T33                                 33u
#define SPARE_T34                                 34u
#define SPARE_T35                                 35u
#define SPARE_T36                                 36u
#define SPARE_T37                                 37u
#define SPARE_T38                                 38u
#define SPARE_T39                                 39u
#define SPARE_T40                                 40u
#define SPARE_T41                                 41u
#define SPARE_T42                                 42u
#define SPARE_T43                                 43u
#define SPARE_T44                                 44u
#define SPARE_T45                                 45u
#define SPARE_T46                                 46u
#define SPARE_T47                                 47u
#define SPARE_T48                                 48u
#define SPARE_T49                                 49u
#define SPARE_T50                                 50u
/*
 * All entries spare upto 255
*/
#define RESERVED_T255                             255u

/*! @} */

/*----------------------------------------------------------------------------
  type definitions
----------------------------------------------------------------------------*/


/*! ===Object Type Definitions===
 This section contains the description, and configuration structures
 used by each object type.
*/

/*! ==DEBUG_DELTAS_T2==
 DEBUG_DELTAS_T2 does not have a structure. It is a int16_t array which
 stores its value in LSByte first and MSByte second.
*/


/*! ==DEBUG_REFERENCES_T3==
 DEBUG_REFERENCES_T3 does not have a structure. It is a uint16_t array which
 stores its value in LSByte first and MSByte second.
*/


/*! ==DEBUG_SIGNALS_T4==
 DEBUG_SIGNALS_T4 does not have a structure. It is a uint16_t array which
 stores its value in LSByte first and MSByte second.
*/


/*! ==GEN_COMMANDPROCESSOR_T6==
 The T6 CommandProcessor allows commands to be issued to the chip
 by writing a non-zero vale to the appropriate register.
*/
/*! \brief
 This structure is used to configure the commandprocessor and should be made
 accessible to the host controller.
*/
typedef struct
{
   uint8_t reset;       /*!< Force chip reset             */
   uint8_t backupnv;    /*!< Force backup to eeprom/flash */
   uint8_t calibrate;   /*!< Force recalibration          */
   uint8_t reportall;   /*!< Force all objects to report  */
   uint8_t RESERVED1;   /*!< Reserved                     */
   uint8_t debugctrl;   /*!< Turn on output of debug data */

} gen_commandprocessor_t6_config_t;


/*! ==GEN_POWERCONFIG_T7==
 The T7 Powerconfig controls the power consumption by setting the
 acquisition frequency.
*/
/*! \brief
 This structure is used to configure the powerconfig and should be made
 accessible to the host controller.
*/
typedef struct
{
   uint8_t idleacqint;        /*!< Idle power mode sleep length in ms           */
   uint8_t actvacqint;        /*!< Active power mode sleep length in ms         */
   uint8_t actv2idleto;       /*!< Active to idle power mode delay length in ms */

} gen_powerconfig_t7_config_t;


/*! ==GEN_ACQUISITIONCONFIG_T8==
 The T8 AcquisitionConfig object controls how the device takes each
 capacitive measurement.
*/
/*! \brief
 This structure is used to configure the acquisitionconfig and should be made
 accessible to the host controller.
*/
typedef struct
{
   uint8_t chrgtime;          /*!< Burst charge time                        */
   uint8_t atchdrift;         /*!< Anti-touch drift compensation period     */
   uint8_t tchdrift;          /*!< Touch drift compensation period          */
   uint8_t driftst;           /*!< Drift suspend time                       */
   uint8_t tchautocal;        /*!< Touch automatic calibration delay in ms  */
   uint8_t sync;              /*!< Measurement synchronisation control      */
   uint8_t atchcalst;         /*!< Anti-touch calibration suspend time      */
   uint8_t atchcalsthr;        /*!< Anti-touch calibration suspend threshold */

} gen_acquisitionconfig_t8_config_t;


/*! ==TOUCH_MULTITOUCHSCREEN_T9==
 The T9 Multitouchscreen is a multi-touch screen capable of tracking upto 8
 independent touches.
*/

/*! \brief
 This structure is used to configure the multitouchscreen and should be made
 accessible to the host controller.
*/
typedef struct
{
   /* Screen Configuration */
   uint8_t ctrl;                /*!< Main configuration field           */

   /* Physical Configuration */
   uint8_t xorigin;             /*!< Object x start position on matrix  */
   uint8_t yorigin;             /*!< Object y start position on matrix  */
   uint8_t xsize;               /*!< Object x size (i.e. width)         */
   uint8_t ysize;               /*!< Object y size (i.e. height)        */

   /* Detection Configuration */
   uint8_t akscfg;              /*!< Adjacent key suppression config     */
   uint8_t blen;                /*!< Burst length for all object channels*/
   uint8_t tchthr;              /*!< Threshold for all object channels   */
   uint8_t tchdi;               /*!< Detect integration config           */

   uint8_t orientate;           /*!< Controls flipping and rotating of touchscreen object  */
   uint8_t mrgtimeout;          /*!< Timeout on how long a touch might ever stay merged -
                                 *   units of 0.2s, used to tradeoff power consumption
                                 *   against being able to detect a touch de-merging early */

   /* Position Filter Configuration */
   uint8_t movhysti;            /*!< Movement hysteresis setting used after touchdown */
   uint8_t movhystn;            /*!< Movement hysteresis setting used once dragging   */
   uint8_t movfilter;           /*!< Position filter setting controlling the rate of  */

   /* Multitouch Configuration */
   uint8_t numtouch;            /*!< The number of touches that the screen will attempt to track             */
   uint8_t mrghyst;             /*!< The hystersis applied on top of the merge threshold to stop oscillation */
   uint8_t mrgthr;              /*!< The threshold for the point when two peaks are considered one touch     */
   uint8_t amphyst;    	        /*!< Amplitude hysteresis                                                    */

   /* Resolution Controls */
   uint16_t xres;               /*!< X resolution */
   uint16_t yres;               /*!< Y resolution */

   uint8_t xloclip;             /*!< X low clipping region width     */
   uint8_t xhiclip;             /*!< X high clipping region width    */
   uint8_t yloclip;             /*!< Y low clipping region width     */
   uint8_t yhiclip;             /*!< Y high clipping region width    */

   uint8_t xedgectrl;           /*!< NULL */
   uint8_t xedgedist;           /*!< NULL */
   uint8_t yedgectrl;           /*!< NULL */
   uint8_t yedgedist;           /*!< NULL */

} touch_multitouchscreen_t9_config_t;


/*! \brief
 This structure is used to configure a slider or wheel and should be made
 accessible to the host controller.
*/
typedef struct
{
   /* Key Array Configuration */
   uint8_t ctrl;           /*!< Main configuration field                         */

   /* Physical Configuration */
   uint8_t xorigin;        /*!< Object x start position on matrix                */
   uint8_t yorigin;        /*!< Object y start position on matrix                */
   uint8_t size;           /*!< Object x size (i.e. width)                       */

   /* Detection Configuration */
   uint8_t akscfg;         /*!< Adjacent key suppression config                  */
   uint8_t blen;           /*!< Burst length for all object channels             */
   uint8_t tchthr;         /*!< Threshold for all object channels                */
   uint8_t tchdi;          /*!< Detect integration config                        */
   uint8_t RESERVED1;      /*!< RESVERED 1                                       */
   uint8_t RESERVED2;      /*!< RESVERED 2                                       */
   uint8_t movhysti;       /*!< Movement hysteresis setting used after touchdown */
   uint8_t movhystn;       /*!< Movement hysteresis setting used once dragging   */
   uint8_t movfilter;      /*!< Position filter setting controlling the rate of  */

} touch_slider_wheel_t11_t12_t13_t14_config_t;


/*! ==TOUCH_KEYARRAY_T15==
 The T15 Keyarray is a key array of upto 32 keys
*/
/*! \brief
 This structure is used to configure the keyarry and should be made
 accessible to the host controller.
*/
typedef struct
{
   /* Key Array Configuration */
   uint8_t ctrl;               /*!< Main configuration field            */

   /* Physical Configuration */
   uint8_t xorigin;            /*!< Object x start position on matrix   */
   uint8_t yorigin;            /*!< Object y start position on matrix   */
   uint8_t xsize;              /*!< Object x size (i.e. width)          */
   uint8_t ysize;              /*!< Object y size (i.e. height)         */

   /* Detection Configuration */
   uint8_t akscfg;             /*!< Adjacent key suppression config     */
   uint8_t blen;               /*!< Burst length for all object channels*/
   uint8_t tchthr;             /*!< Threshold for all object channels   */
   uint8_t tchdi;              /*!< Detect integration config           */
   uint8_t RESERVED1;          /*!< RESVERED 1                          */
   uint8_t RESERVED2;          /*!< RESVERED 2                          */

} touch_keyarray_t15_config_t;

/*! ==SPT_GPIOPWM_T19==
 The T19 GPIOPWM object provides input, output, wake on change and PWM support
for one port.
*/
/*! \brief
 This structure is used to configure the GPIOPWM object and should be made
 accessible to the host controller.
*/
typedef struct
{
   /* GPIOPWM Configuration */
   uint8_t ctrl;             /*!< Main configuration field                       */
   uint8_t reportmask;       /*!< Event mask for generating messages to the host */
   uint8_t dir;              /*!< Port DIR register                              */
   uint8_t pullup;           /*!< Port pull-up per pin enable register           */
   uint8_t out;              /*!< Port OUT register                              */
   uint8_t wake;             /*!< Port wake on change enable register            */
   uint8_t pwm;              /*!< Port pwm enable register                       */
   uint8_t per;              /*!< PWM period (min-max) percentage                */
   uint8_t duty[4];          /*!< PWM duty cycles percentage                     */

} spt_gpiopwm_t19_config_t;


/*! ==PROCI_GRIPFACESUPPRESSION_T20==
 The T20 GRIPFACESUPPRESSION object provides grip bounday suppression and
 face suppression
*/
/*! \brief
 This structure is used to configure the GRIPFACESUPPRESSION object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t ctrl;             /*!< Enable/Disable grip and/or face suppression.           */
   uint8_t xlogrip;          /*!< Grip suppression bottom boundary height                */
   uint8_t xhigrip;          /*!< Grip suppression top boundary height                   */
   uint8_t ylogrip;          /*!< Grip suppression left boundary width                   */
   uint8_t yhigrip;          /*!< Grip suppression right boundary width                  */
   uint8_t maxtchs;          /*!< Maximum touches                                        */
   uint8_t RESERVED1;        /*!< RESVERED 1                                             */
   uint8_t szthr1;           /*!< Face suppression size threshold for touch              */
   uint8_t szthr2;           /*!< Face suppression size threshold for approaching target */
   uint8_t shpthr1;          /*!< Face suppression shape threshold 1                     */
   uint8_t shpthr2;          /*!< Face suppression shape threshold 2                     */
   uint8_t supextto;         /*!< Suppression extension timeout                          */

} proci_gripfacesuppression_t20_config_t;


/*! ==PROCG_NOISESUPPRESSION_T22==
 The T22 NOISESUPPRESSION object provides ferquency hopping acquire control,
 outlier filtering and grass cut filtering.
*/
/*! \brief
 This structure is used to configure the NOISESUPPRESSION object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t ctrl;              /*!< Enable/Disable the noise suppression object                     */
   uint8_t RESERVED1;         /*!< RESVERED 1                                                      */
   uint8_t RESERVED2;         /*!< RESVERED 2                                                      */
   int16_t gcaful;            /*!< GCAF upper limit                                                */
   int16_t gcafll;            /*!< GCAG lower limit                                                */
   uint8_t actvgcafvalid;     /*!< Number of GCAF samples in active mode for a signal to be valid. */
   uint8_t noisethr;          /*!< Noise threshold                                                 */
   uint8_t RESERVED3;         /*!< RESVERED 3                                                      */
   uint8_t freqhopscale;      /*!< Scaling factor                                                  */
   uint8_t freq0;             /*!< Burst frequency 0                                               */
   uint8_t freq1;             /*!< Burst frequency 1                                               */
   uint8_t freq2;             /*!< Burst frequency 2                                               */
   uint8_t freq3;             /*!< Burst frequency 3                                               */
   uint8_t freq4;             /*!< Burst frequency 4                                               */
   uint8_t idlegcafvalid;     /*!< Number of GCAF samples in idle mode for a signal to be valid.   */
   uint8_t idlegcafquite;
   uint8_t actvgcafquiet;

} procg_noisesuppression_t22_config_t;


/*! ==TOUCH_PROXIMITY_T23==
 The T23 TOUCH PROXIMITY object provides is used to configure a proximity sensor
 as a rectangular matrix of up to 16 XY channels.
*/
/*! \brief
 This structure is used to configure the TOUCH PROXIMITY object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t  ctrl;            /*!< Enable/Disable the proximity sensor for use and determines
                              *   which messages will be reported.                              */
   uint8_t  oxorigin;        /*!< X line start position of object                               */
   uint8_t  yorigin;         /*!< Y line start position of object                               */
   uint8_t  xsize;           /*!< Number of X lines the object occupies                         */
   uint8_t  ysize;           /*!< Number of Y lines the object occupies                         */
   uint8_t  RESERVED1;       /*!< RESVERED 1                                                    */
   uint8_t  blen;            /*!< Sets the gain of the analog circuits in front of the ADC      */
   int16_t  tchthr;          /*!< Touch threshold                                               */
   uint8_t  tchdi;           /*!< Touch detect integration                                      */
   uint8_t  average;         /*!< Acquisition cycles to be averaged (power of 2)                */
   int16_t  rate;            /*!< Rate of change before deltas reported                         */

} touch_proximity_t23_config_t;


/*! ==PROCI_ONETOUCHGESTUREPROCESSOR_T24==
 The T24 ONETOUCHGESTUREPROCESSOR object provides gesture recognition from
 touchscreen touches.
*/
/*! \brief
 This structure is used to configure the ONETOUCHGESTUREPROCESSOR object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t  ctrl;          /*!< Enable/Disable the use of the One-touch Gesture Processor object */
   uint8_t  numgest;       /*!< Maximum number of reported gestures                              */
   uint16_t gesten;        /*!< Enables one-touch gestures and reporting                         */
   uint8_t  pressproc;     /*!< Control the type of press events that can be processed by the device. */
   uint8_t  tapto;         /*!< Tap timeout                                                      */
   uint8_t  flickto;       /*!< Flick timeout                                                    */
   uint8_t  dragto;        /*!< Drage timeout                                                    */
   uint8_t  spressto;      /*!< Short Press timeout                                              */
   uint8_t  lpressto;      /*!< Long Press timeout                                               */
   uint8_t  rptpressto;    /*!< Repeat Press timeout                                             */
   uint16_t flickthr;      /*!< Flick threshold                                                  */
   uint16_t dragthr;       /*!< Drag threshold                                                   */
   uint16_t tapthr;        /*!< Tap threshold                                                    */
   uint16_t throwthr;      /*!< Throw threshold                                                  */

} proci_onetouchgestureprocessor_t24_config_t;


/*! ==SPT_SELFTEST_T25==
 The T25 SEFLTEST object provides self testing routines
*/
/*! \brief
 This structure is used to configure the SELFTEST object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t  ctrl;             /*!< Enable/Disable that allows the object to run tests           */
   uint8_t  cmd;              /*!< Test code of test to run                                     */
   uint16_t upsiglim_0;       /*!< Higher signal limit for touch object 0                       */
   uint16_t losiglim_0;       /*!< Lower signal limit for touch object 0                        */
   uint16_t upsiglim_1;       /*!< Higher signal limit for touch object 1                       */
   uint16_t losiglim_1;       /*!< Lower signal limit for touch object 1                        */
   uint16_t upsiglim_2;       /*!< Higher signal limit for touch object 2                       */
   uint16_t losiglim_3;       /*!< Lower signal limit for touch object 2                        */

} spt_selftest_t25_config_t;


/*! ==PROCI_TWOTOUCHGESTUREPROCESSOR_T27==
 The T27 TWOTOUCHGESTUREPROCESSOR object provides gesture recognition from
 touchscreen touches.
*/
/*! \brief
 This structure is used to configure the TWOTOUCHGESTUREPROCESSOR object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t  ctrl;          /*!< Bit 0 = object enable, bit 1 = report enable               */
   uint8_t  numgest;       /*!< Runtime control for how many two touch gestures to process */
   uint8_t  RESERVED1;     /*!< RESVERED 1                                                 */
   uint8_t  gesten;        /*!< Control for turning particular gestures on or off          */
   uint8_t  rotatethr;     /*!< Rotate threshold                                           */
   uint16_t zoomthr;       /*!< Zoom threshold                                             */

} proci_twotouchgestureprocessor_t27_config_t;


/*! ==SPT_CTECONFIG_T28==
 The T28 CTECONFIG object provides controls for the CTE.
*/
/*! \brief
 This structure is used to configure the CTECONFIG object and
 should be made accessible to the host controller.
*/
typedef struct
{
   uint8_t ctrl;              /*!< Ctrl field reserved for future expansion     */
   uint8_t cmd;               /*!< Cmd field for sending CTE commands           */
   uint8_t mode;              /*!< CTE mode configuration field                 */
   uint8_t idlegcafdepth;     /*!< GCAF sampling window in idle mode            */
   uint8_t actvgcafdepth;     /*!< GCAF sampling window in active mode          */

} spt_cteconfig_t28_config_t;

#endif /* STD_OBJECTS */
