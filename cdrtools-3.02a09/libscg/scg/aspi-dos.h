/* @(#)aspi-dos.h	1.3 10/08/08 J. Schilling */
#ifndef	__ASPI16_H_
#define	__ASPI16_H_

#define	PACKED	__attribute__((packed))
#define	FAR
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

//*****************************************************************************
//	%%% SCSI MISCELLANEOUS EQUATES %%%
//*****************************************************************************

#define	SENSE_LEN			14	// Default sense buffer length
#define	SRB_DIR_SCSI			0x00	// Direction determined by SCSI
#define	SRB_POSTING			0x01	// Enable ASPI posting
#define	SRB_ENABLE_RESIDUAL_COUNT	0x04	// Enable residual byte count reporting
#define	SRB_DIR_IN			0x08	// Transfer from SCSI target to host
#define	SRB_DIR_OUT			0x10	// Transfer from host to SCSI target

//*****************************************************************************
//      %%% ASPI Command Definitions %%%
//*****************************************************************************

#define	SC_HA_INQUIRY			0x00	// Host adapter inquiry
#define	SC_GET_DEV_TYPE			0x01	// Get device type
#define	SC_EXEC_SCSI_CMD		0x02	// Execute SCSI command
#define	SC_ABORT_SRB			0x03	// Abort an SRB
#define	SC_RESET_DEV			0x04	// SCSI bus device reset
#define	SC_SET_HA_PARMS			0x05	// Set HA parameters
#define	SC_GET_DISK_INFO		0x06	// Get Disk information

//*****************************************************************************
//      %%% SRB Status %%%
//*****************************************************************************

#define	SS_PENDING			0x00	// SRB being processed
#define	SS_COMP				0x01	// SRB completed without error
#define	SS_ABORTED			0x02	// SRB aborted
#define	SS_ABORT_FAIL			0x03	// Unable to abort SRB
#define	SS_ERR				0x04	// SRB completed with error

#define	SS_INVALID_CMD			0x80	// Invalid ASPI command
#define	SS_INVALID_HA			0x81	// Invalid host adapter number
#define	SS_NO_DEVICE			0x82	// SCSI device not installed

//*****************************************************************************
//      %%% Host Adapter Status %%%
//*****************************************************************************

#define	HASTAT_OK			0x00	// Host adapter did not detect an
						// error
#define	HASTAT_SEL_TO			0x11	// Selection Timeout
#define	HASTAT_DO_DU			0x12	// Data overrun data underrun
#define	HASTAT_BUS_FREE			0x13	// Unexpected bus free
#define	HASTAT_PHASE_ERR		0x14	// Target bus phase sequence
						// failure
#define	HASTAT_TIMEOUT			0x09	// Timed out while SRB was
						// waiting to beprocessed.
#define	HASTAT_COMMAND_TIMEOUT		0x0B	// Adapter timed out processing SRB.
#define	HASTAT_MESSAGE_REJECT		0x0D	// While processing SRB, the
						// adapter received a MESSAGE
#define	HASTAT_BUS_RESET		0x0E	// A bus reset was detected.
#define	HASTAT_PARITY_ERROR		0x0F	// A parity error was detected.
#define	HASTAT_REQUEST_SENSE_FAILED	0x10	// The adapter failed in issuing

typedef struct {

	BYTE	Cmd;				// 00/000 ASPI command code = SC_EXEC_SCSI_CMD
	BYTE	Status;				// 01/001 ASPI command status byte
	BYTE	HaId;				// 02/002 ASPI host adapter number
	BYTE	Flags;				// 03/003 ASPI request flags
	DWORD	Hdr_Rsvd;			// 04/004 Reserved, MUST = 0

	union {

	struct {

		BYTE	Count;			// 08/008 Number of host adapters present
		BYTE	SCSI_ID;		// 09/009 SCSI ID of host adapter
		BYTE	ManagerId[16];		// 0A/010 String describing the manager
		BYTE	Identifier[16];		// 1A/026 String describing the host adapter
		BYTE	Unique[16];		// 2A/042 Host Adapter Unique parameters
		BYTE	ExtBuffer[8];		// 3A/058 Extended inquiry data

	} PACKED HAInquiry;

	struct {

		BYTE	Target;			// 08/008 Target's SCSI ID
		BYTE	Lun;			// 09/009 Target's LUN number
		BYTE	DeviceType;		// 0A/010 Target's peripheral device type

	} PACKED GetDeviceType;

	struct {

		BYTE	Target;			// 08/008 Target's SCSI ID
		BYTE	Lun;			// 09/009 Target's LUN number
		DWORD	BufLen;			// 0A/010 Data Allocation Length
		BYTE	SenseLen;		// 0E/014 Sense Allocation Length
		BYTE	FAR *BufPointer;	// 0F/015 Data Buffer Pointer
		DWORD	Rsvd1;			// 13/019 Reserved, MUST = 0
		BYTE	CDBLen;			// 17/023 CDB Length = 6/10/12
		BYTE	HaStat;			// 18/024 Host Adapter Status
		BYTE	TargStat;		// 19/025 Target Status
		void	FAR *PostProc;		// 1A/026 Post routine
		BYTE	Rsvd2[34];		// 1E/030 Reserved, MUST = 0

		union {

		struct {

			BYTE	CDBByte[6];		// 40/064 SCSI CDB
			BYTE	SenseArea[SENSE_LEN+2];	// 46/070 Request Sense buffer

		} PACKED _6;

		struct {

			BYTE	CDBByte[10];		// 40/064 SCSI CDB
			BYTE	SenseArea[SENSE_LEN+2];	// 4A/074 Request Sense buffer

		} PACKED _10;

		struct {

			BYTE	CDBByte[12];		// 40/064 SCSI CDB
			BYTE	SenseArea[SENSE_LEN+2];	// 4C/076 Request Sense buffer

		} PACKED _12;

		} PACKED CmdLen;

	} PACKED ExecSCSICmd;

	struct {

		void	FAR	*SRBToAbort;	// 08/008 Pointer to SRB to abort

	} PACKED Abort;

	struct {
		BYTE	Target;		// 08/008 Target's SCSI ID
		BYTE	Lun;		// 09/009 Target's LUN number
		BYTE	ResetRsvd1[14];	// 0A/010 Reserved, MUST = 0
		BYTE	HaStat;		// 18/024 Host Adapter Status
		BYTE	TargStat;	// 19/025 Target Status
		void	FAR *PostProc;	// 1A/026 Post routine
		BYTE	ResetRsvd2[34];	// 1E/030 Reserved, MUST = 0
	} Reset;
	} PACKED Type;

} PACKED SRB;

#endif /* __ASPI16_H_ */
