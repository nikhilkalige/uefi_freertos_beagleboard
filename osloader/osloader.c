#include  <Uefi.h>
#include  <Library/UefiBootServicesTableLib.h>
#include  <Library/UefiRuntimeServicesTableLib.h>
#include  <Library/DevicePathLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/ShellLib.h>
#include  <Library/ArmLib.h>
#include  <Library/UefiLib.h>
#include  <Protocol/LoadedImage.h>
#include  <Protocol/LoadFile.h>
#include  <Library/UefiApplicationEntryPoint.h>
#include  "elf_common.h"
#include  "elf32.h"

#define CheckStatus(Status, Code) {\
  if(EFI_ERROR(Status)){\
    Print(L"Error: Status = %d, LINE=%d in %s\n", (Status), __LINE__, __func__);\
    Code;\
  }\
}

// Freertos function prototype
typedef VOID (*freertos_elf)(EFI_RUNTIME_SERVICES* runtime);

/** Guid value for saving variable */
EFI_GUID rtos_var = {
    0xAA, 0xBB, 0xCC, {
        0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B, 0x8C
    }
};

STATIC EFI_STATUS PreparePlatformHardware(VOID)
{
    // Turn off MMU
    ArmDisableMmu();
    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI LoadFileByName (
    IN  CONST CHAR16  *FileName,
    OUT UINT8     **FileData,
    OUT UINTN     *FileSize
)
{
    EFI_STATUS          Status;
    SHELL_FILE_HANDLE   FileHandle;
    EFI_FILE_INFO       *Info;
    UINTN               Size;
    UINT8               *Data;

    Status = ShellOpenFileByName(FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
    CheckStatus(Status, return(Status));

    Info = ShellGetFileInfo(FileHandle);
    Size = (UINTN)Info->FileSize;
    FreePool(Info);

    Data = AllocateRuntimeZeroPool(Size);
    if(Data == NULL) {
        Print(L"Error: AllocateRuntimeZeroPool failed\n");
        return(EFI_OUT_OF_RESOURCES);
    }

    // Read file into Buffer
    Status = ShellReadFile(FileHandle, &Size, Data);
    CheckStatus(Status, return(Status));

    // Close file
    Status = ShellCloseFile(&FileHandle);
    CheckStatus(Status, return(Status));

    *FileSize = Size;
    *FileData = Data;

    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI ElfLoadSegment (
    IN  CONST VOID  *ElfImage,
    OUT VOID    **EntryPoint
)
{
  Elf32_Ehdr   *ElfHdr;
  UINT8        *ProgramHdr;
  Elf32_Phdr   *ProgramHdrPtr;
  UINTN        Index;

  ElfHdr = (Elf32_Ehdr *)ElfImage;
  ProgramHdr = (UINT8 *)ElfImage + ElfHdr->e_phoff;

  // Load every loadable ELF segment into memory
    for(Index = 0; Index < ElfHdr->e_phnum; Index++) {
        ProgramHdrPtr = (Elf32_Phdr *)ProgramHdr;

        if(ProgramHdrPtr->p_type == PT_LOAD) {
            VOID  *FileSegment;
            VOID  *MemSegment;
            VOID  *ExtraZeroes;
            UINTN ExtraZeroesCount;

            // Load the segment in memory
            FileSegment = (VOID *)((UINTN)ElfImage + ProgramHdrPtr->p_offset);
            MemSegment = (VOID *)ProgramHdrPtr->p_vaddr;
            gBS->CopyMem(MemSegment, FileSegment, ProgramHdrPtr->p_filesz);

            ExtraZeroes = (UINT8 *)MemSegment + ProgramHdrPtr->p_filesz;
            ExtraZeroesCount = ProgramHdrPtr->p_memsz - ProgramHdrPtr->p_filesz;
            if(ExtraZeroesCount > 0) {
                gBS->SetMem(ExtraZeroes, 0x00, ExtraZeroesCount);
            }
        }
        ProgramHdr += ElfHdr->e_phentsize;
    }

    *EntryPoint = (VOID *)ElfHdr->e_entry;
    return (EFI_SUCCESS);
}


EFI_STATUS EFIAPI UefiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS             Status;
    CHAR16                 *FileName = L"fs0:\\rtosdemo.elf";
    UINTN                  FileSize;
    UINT8                  *FileData;
    VOID                   *EntryPoint = NULL;
    UINTN                  MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR  *MemoryMap = NULL;
    UINTN                  MapKey;
    UINTN                  DescriptorSize;
    UINT32                 DescriptorVersion;
    UINT16                 var_value;
    UINTN                  len;

    /** Freertos function ptr */
    freertos_elf start_elf;

    // Load the ELF file to buffer
    Print(L"Loading File %s\n", FileName);
    Status = LoadFileByName(FileName, &FileData, &FileSize);
    CheckStatus(Status, return(-1));

    Print(L"Upload section to memory\n");
    Status = ElfLoadSegment(FileData, &EntryPoint);
    CheckStatus(Status, return(-1));

    Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey,
            &DescriptorSize, &DescriptorVersion);
    if (Status != EFI_BUFFER_TOO_SMALL) {
        CheckStatus(Status, return(-1));
    }

    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool(MemoryMapSize);
    if (MemoryMap == NULL) {
        Print(L"Error: Allocate Pages failed\n");
        return(-1);
    }

    Print(L"ExitBootServices and Execute the program freertos\n");

    Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey,
            &DescriptorSize, &DescriptorVersion);
    CheckStatus(Status, return(-1));

    Status = gBS->ExitBootServices(gImageHandle, MapKey);
    CheckStatus(Status, return(-1));

    /** Set value to store */
    var_value = 31;
    Status = gRT->SetVariable(L"FREERTOS", &rtos_var,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(UINT16),
        &var_value
    );
    Status = PreparePlatformHardware();
    CheckStatus(Status, return(-1));

    var_value = 67;
    Status = gRT->GetVariable(L"FREERTOS", &rtos_var, NULL, &len, &var_value);

    if (EFI_ERROR (Status)) {
        var_value = 32;
    }
    // Jump to the entrypoint of executable
    // goto *EntryPoint;
    start_elf = (freertos_elf)EntryPoint;
    start_elf(gRT);
    return 0;
}

