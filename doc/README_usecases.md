# UTFS Use Cases

### Typical use case
The typical use case for UTFS is for systems which would like to collect data in to different data structures, as 'files', and have those files all loaded and saved at the same time.

Sequence of operation:

- System boots
- Call the UTFS init() function to initialize driver
- All code modules shall 'register' their UTFS file with the UTFS driver
- Hardware setup, non-volatile medium access ready
- Call UTFS load() function to load all data
- Call any setup() functions for the code modules
- All data is loaded and ready to use


## Example:

```
struct system_data {
  char serialnumber[12];
  char modelnumber[12];
}

struct application_data {
  uint32_t last_updated_timestamp;
  uint8_t current_state;
  uint8_t error_count;
  uint16_t app_timeout_limit;
}

void main()
{
  ...
  utfs_file_t sys_file;
  utfs_file_t app_file;

  utfs_init();

  sprintf(sys_file.filename,"system");
  sys_file.data = &(system_data);
  sys_file.size = sizeof(system_data);
  utfs_register(&sys_file, UTFS_NOFLAGS, UTFS_NOOPT);

  sprintf(app_file.filename,"appdata");
  app_file.data = &(application_data);
  app_file.size = sizeof(application_data);
  utfs_register(&app_file, UTFS_NOFLAGS, UTFS_NOOPT);

  system_hardware_setup();


  utfs_load();


  // Check data loaded to see if it needs default values, defualt values, etc
  if(sys_file.singature!=SIGNATURE_VALUE_SYS) // Invalid data, default data
  if(app_file.singature!=SIGNATURE_VALUE_APP) // Invalid data, default data

  // data ready to use
  while(1){
    ...

    if(command=="setserial"){
        snprintf(system_data.serialnumber,sizeof(system_data.serialnumber),"%s",payload);
        utfs_save(); // All data is written to flash
    }


    ...
  }

}

```

# UTFS corner cases

### Flags and corner cases

Flags are a proposed feature of the UTFS interface.  Flags can provide flexibility for integrating with existing systems that don't follow the typical UTFS paradigm.

At the time of writing, these flags are not a finalized part of the UTFS logic.

**FLAG_LOADEXPLICT**
This flag exists to allow the system to load a specific file on-demand, and not at the typical call to the utfs_load() function, which would load all files.

This flag should be used with caution, as any call to the utfs_save() function will write the current data to the non-volatile medium.  This can cause other files to move the location of the current file and overwrite the data.  Or this could cause blank data to be written, if the explicit file is not loaded yet when the save is called.


**Flag_SAVEEXPLICIT**
This flag exists to allow the system to save a specific file on-demand, and not at the typical call to the UTFS save() function, which would save all files.

This flag should be used with caution, as a call to utfs_save() followed by a restart could cause any data in the current RAM of the explicit file to not be saved to the medium.

Another corner case is where an explicit file grows after the 'structure' of the other files is written to the medium, and then the explicit file is written.  This could cause the explicit file to overwrite the files after it on the medium. This is due to utfs_save_file() using the current size of the data vs the size of the structure written to medium.  This shall be discussed and addressed before this flag is approved as part of UTFS.
