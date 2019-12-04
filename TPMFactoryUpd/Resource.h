/**
 *	@brief		Declares resource definitions
 *	@details
 *	@file		Resource.h
 *	@copyright	Copyright 2014 - 2017 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//---------------- Menu Header -------------
#define MENU_NEWLINE								L""
#define MENU_SEPARATOR								L"  **********************************************************************"
#define MENU_SEPARATOR_LINE							L"  ----------------------------------------------------------------------"
#define MENU_SEPARATOR_DOUBLELINE					L"  ======================================================================"
#define MENU_HEADLINE								L"  *    %ls   %ls   %ls %ls    *" /* Use with format IFX_BRAND, TOOL_NAME, MENU_VERSION and APP_VERSION */
#define MENU_VERSION								L"Ver"
#define MENU_EMPTY_BOX								L"  *                                                                    *"

//--------- Response Header -----------
#define RES_UNKNOWN_ACTION							L"  *    Unknown Action                                                  *"
#define RES_TPM_ERROR_EXPLANATION					L"  Explanation:    "
#define RES_TPM_ERROR_EXPLANATION_PRESET			L"                  "
#define RES_MAX_LINE_SIZE							72
#define RES_ERROR_CODE								L"  Error Code:     0x%.8X"
#define RES_ERROR_MESSAGE							L"  Message:        %ls"
#define RES_ERROR_MESSAGE_PRESET					L"  Message:        "
#define RES_ERROR_DETAILS							L"  Details:        0x%.8X"
#define RES_ERROR_LOGFILE_HINT						L"See the log file (%ls) for further information."
#define RES_ERROR_LOGFILE_UNAVAILABLE				L"The log file could not be opened or created. For further information please configure an accessible and writable log path."
#define RES_HEX_VIEW_PRESET							L"       "
#define RES_ERROR_INFO								L"  *    Error Information                                               *"

//-----------------TpmInfo response-----------------
#define RES_TPM_INFO_INFORMATION					L"       TPM information:"
#define RES_TPM_INFO_DASHED_LINE					L"       ----------------"
#define RES_TPM_INFO_FIRMWARE_VALID					L"       Firmware valid                    :    %ls"
#define RES_TPM_INFO_TPM_FAMILY						L"       TPM family                        :    %ls"
#define RES_TPM_INFO_TPM_VERSION					L"       TPM firmware version              :    %ls"
#define RES_TPM_INFO_REMAINING_UPDATES_NUMBER		L"       Remaining updates                 :    %d"
#define RES_TPM_INFO_REMAINING_UPDATES_STRING		L"       Remaining updates                 :    %ls"
#define RES_TPM_ENABLED								L"       TPM enabled                       :    %ls"
#define RES_TPM_ACTIVATED							L"       TPM activated                     :    %ls"
#define RES_TPM_HAS_OWNER							L"       TPM owner set                     :    %ls"
#define RES_TPM_DEFERREDPP							L"       TPM deferred physical presence    :    %ls"
#define RES_TPM_PLATFORM_AUTH						L"       TPM platformAuth                  :    %ls"
#define RES_TPM_INFO_1_2							L"1.2"
#define RES_TPM_INFO_2_0							L"2.0"
#define RES_TPM_INFO_N_A							L"N/A"
#define RES_TPM_INFO_N_A_RESTART					L"N/A - System restart required"
#define RES_TPM_INFO_YES							L"Yes"
#define RES_TPM_INFO_NO								L"No"
#define RES_TPM_INFO_NO_NOT_SETTABLE				L"No (Not settable)"
#define RES_TPM_INFO_NO_SETTABLE					L"No (Settable)"
#define RES_TPM_INFO_EMPTY_BUFFER					L"Empty Buffer"
#define RES_TPM_INFO_NOT_EMPTY_BUFFER				L"Not Empty Buffer"
#define RES_TPM_INFO_PHDISABLED						L"Platform hierarchy disabled"
#define RES_TPM_INFO_IN_FAILURE_MODE_1				L"The TPM2.0 is in failure mode. Not all TPM information is available."
#define RES_TPM_INFO_IN_FAILURE_MODE_2				L"The test result is: %ls"
#define RES_TPM_INFO_IN_FAILURE_MODE_3				L"Restart the system and try again."
#define RES_TPM_INFO_IN_SELFTEST_FAILED_MODE_1		L"The TPM1.2 is in self-test failed mode. Not all TPM information is available."
#define RES_TPM_INFO_IN_SELFTEST_FAILED_MODE_2		L"The test result is: %ls"
#define RES_TPM_USED_FIRMWARE_FILE					L"       Selected firmware image:\n       %ls"
#define RES_TPM_ALREADY_UP_TO_DATE					L"       The current TPM firmware version is already up to date!"

//---------------- TpmUpdate response -----------------
#define RES_TPM_UPDATE_INFORMATION					L"       TPM update information:"
#define RES_TPM_UPDATE_DASHED_LINE					L"       -----------------------"
#define RES_TPM_UPDATE_TPM_FAMILY_AFTER				L"       TPM family after update           :    %ls"
#define RES_TPM_UPDATE_TPM_VERSION_AFTER			L"       TPM firmware version after update :    %ls"
#define RES_TPM_UPDATE_UPDATABLE					L"       New firmware valid for TPM        :    %ls"
#define RES_TPM_UPDATE_PREPARE						L"       Preparation steps:"
#define RES_TPM_UPDATE_PREPARE_POLICY				L"       TPM2.0 policy session created to authorize the update."
#define RES_TPM_UPDATE_PREPARE_POLICY_FAIL			L"       TPM2.0 policy session creation failed."
#define RES_TPM_UPDATE_PREPARE_SKIP					L"       Skipped"
#define RES_TPM_UPDATE_PREPARE_PP					L"       TPM1.2 Deferred Physical Presence preparation successful."
#define RES_TPM_UPDATE_PREPARE_PP_EXT				L"       TPM1.2 Deferred Physical Presence has been set externally."
#define RES_TPM_UPDATE_PREPARE_PP_FAIL				L"       TPM1.2 Physical Presence is locked and Deferred Physical\n       Presence is not set. The firmware cannot be updated."
#define RES_TPM_UPDATE_PREPARE_TAKEOWNERSHIP		L"       TPM1.2 Ownership preparation was successful."
#define	RES_TPM_UPDATE_PREPARE_TAKEOWNERSHIP_FAIL	L"       TPM1.2 Ownership preparation failed."
#define RES_TPM_UPDATE_DO_NOT_TURN_OFF				L"    DO NOT TURN OFF OR SHUT DOWN THE SYSTEM DURING THE UPDATE PROCESS!"
#define RES_TPM_UPDATE_UPDATE						L"       Updating the TPM firmware ..."
#define RES_TPM_UPDATE_SUCCESS						L"       TPM Firmware Update completed successfully."
#define RES_TPM_UPDATE_FAIL							L"       TPM Firmware Update failed."
#define RES_TPM_UPDATE_PROGRESS						L"       Completion: %d %%\r"
#define RES_TPM_UPDATE_FACTORYDEFAULT				L"       TPM chip state after update       :    reset to factory defaults"

//---------------- Tpm12_ClearOwnership response ------------
#define RES_TPM12_CLEAR_OWNER_INFORMATION			L"       TPM1.2 Clear Ownership:"
#define RES_TPM12_CLEAR_OWNER_DASHED_LINE			L"       -----------------------"
#define RES_TPM12_CLEAR_OWNER_SUCCESS				L"       Clear TPM1.2 Ownership operation completed successfully."
#define RES_TPM12_CLEAR_OWNER_FAILED				L"       Clear TPM1.2 Ownership operation failed. (0x%.8X)"

// --------------- Command line options ---------------------
#define CMD_HELP									L"help"
#define CMD_HELP_ALT								L"?"
#define CMD_INFO									L"info"
#define CMD_UPDATE									L"update"
#define CMD_UPDATE_OPTION_TPM12_DEFERREDPP			L"tpm12-PP"
#define CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP		L"tpm12-takeownership"
#define CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH	L"tpm20-emptyplatformauth"
#define CMD_UPDATE_OPTION_CONFIG_FILE				L"config-file"
#define CMD_FIRMWARE								L"firmware"
#define CMD_LOG										L"log"
#define CMD_TPM12_CLEAROWNERSHIP					L"tpm12-clearownership"
#define CMD_ACCESS_MODE								L"access-mode"
#define CMD_CONFIG									L"config"
#define CMD_DRY_RUN									L"dry-run"
#define CMD_IGNORE_ERROR_ON_COMPLETE				L"ignore-error-on-complete"

// --------------- Help Output ---------------------
#define HELP_LINE1		L"Call: TPMFactoryUpd [parameter] [parameter] ..."
#define HELP_LINE2		L" "
#define HELP_LINE3		L"Parameters:"
#define HELP_LINE4		L"\n-%ls or -%ls" /* Use with format CMD_HELP_ALT and CMD_HELP */
#define HELP_LINE5		L"  Displays a short help page for the operation of TPMFactoryUpd (this screen)."
#define HELP_LINE6		L"  Cannot be used with any other parameter."
#define HELP_LINE7		L"\n-%ls" /* Use with format CMD_INFO */
#define HELP_LINE8		L"  Displays TPM information related to TPM Firmware Update."
#define HELP_LINE9		L"  Cannot be used with -%ls, -%ls, -%ls or -%ls parameter." /* Use with format CMD_FIRMWARE, CMD_UPDATE, CMD_CONFIG and CMD_TPM12_CLEAROWNERSHIP */
#define HELP_LINE10		L"\n-%ls <update-type>" /* Use with format CMD_UPDATE */
#define HELP_LINE11		L"  Updates a TPM with <update-type>."
#define HELP_LINE12		L"  Possible values for <update-type> are:"
#define HELP_LINE13		L"   %ls - TPM1.2 with Physical Presence or Deferred Physical Presence." /* Use with format CMD_UPDATE_OPTION_TPM12_DEFERREDPP */
#define HELP_LINE14		L"   %ls - TPM1.2 with TPM Ownership taken by TPMFactoryUpd." /* Use with format CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP */
#define HELP_LINE15		L"   %ls - TPM2.0 with platformAuth set to Empty Buffer." /* Use with format CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH */
#define HELP_LINE16		L"   %ls - Updates either a TPM1.2 or TPM2.0 to the firmware version"
#define HELP_LINE17     L"                 configured in the configuration file. Requires the -config parameter." /* Use with format CMD_UPDATE_OPTION_CONFIG_FILE */
#define HELP_LINE18		L"  Cannot be used with -%ls or -%ls parameter." /* Use with format CMD_INFO and CMD_TPM12_CLEAROWNERSHIP*/
#define HELP_LINE19		L"\n-%ls <firmware-file>" /* Use with format CMD_FIRMWARE */
#define HELP_LINE20		L"  Specifies the path to the firmware image to be used for TPM Firmware Update."
#define HELP_LINE21		L"  Required if -%ls parameter is given with values tpm*." /* Use with format CMD_UPDATE*/
#define HELP_LINE22		L"  Cannot be used with -%ls, -%ls or -%ls parameter." /* Use with format CMD_INFO, CMD_CONFIG and CMD_TPM12_CLEAROWNERSHIP*/
#define HELP_LINE23		L"\n-%ls <config-file>" /* Use with format CMD_CONFIG */
#define HELP_LINE24		L"  Specifies the path to the configuration file to be used for TPM Firmware Update."
#define HELP_LINE25		L"  Required if -%ls parameter is given with value %ls." /* Use with format CMD_UPDATE, CMD_UPDATE_OPTION_CONFIG_FILE */
#define HELP_LINE26		L"  Cannot be used with -%ls, -%ls or -%ls parameter." /* Use with format CMD_INFO, CMD_FIRMWARE and CMD_TPM12_CLEAROWNERSHIP*/
#define HELP_LINE27		L"\n-%ls [<log-file>]" /* Use with format CMD_LOG */
#define HELP_LINE28		L"  Optional parameter. Activates logging for TPMFactoryUpd to the log file"
#define HELP_LINE29		L"  specified by <log-file>. Default value .\\TPMFactoryUpd.log is used if"
#define HELP_LINE30		L"  <log-file> is not given."
#define HELP_LINE31		L"  Note: total path and file name length must not exceed 260 characters"
#define HELP_LINE32		L"\n-%ls" /* Use with format CMD_TPM12_CLEAROWNERSHIP */
#define HELP_LINE33		L"  Clears the TPM Ownership taken by TPMFactoryUpd."
#define HELP_LINE34		L"  Cannot be used with -%ls, -%ls, -%ls or -%ls parameter." /* Use with format CMD_FIRMWARE, CMD_UPDATE, CMD_CONFIG and CMD_INFO */
#define HELP_LINE35		L"\n-%ls <mode> <path>" /* Use with format CMD_ACCESS_MODE */
#define HELP_LINE36		L"  Optional parameter. Sets the mode the tool should use to connect to"
#define HELP_LINE37		L"  the TPM device."
#define HELP_LINE38		L"  Possible values for <mode> are:"
#define HELP_LINE39		L"  1 - Memory based access (default value, only supported on x86 based systems"
#define HELP_LINE40		L"      with PCH TPM support)"
#define HELP_LINE41		L"  3 - Linux TPM driver. The <path> option can be set to define a device path"
#define HELP_LINE42		L"      (default value: /dev/tpm0)"
#define HELP_LINE43		L"\n-%ls" /* use with format CMD_DRY_RUN */
#define HELP_LINE44		L"  Optional parameter. Do everything except actually updating the image."
#define HELP_LINE45		L"\n-%ls" /* use with format CMD_IGNORE_ERROR_ON_COMPLETE */
#define HELP_LINE46		L"  Optional parameter. Ignores TPM_FAIL errors from FieldUpgradeComplete."

//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by TPMFactoryUpd.rc

// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE		101
#define _APS_NEXT_COMMAND_VALUE			40001
#define _APS_NEXT_CONTROL_VALUE			1001
#define _APS_NEXT_SYMED_VALUE			101
#endif
#endif
