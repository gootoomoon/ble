
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "list.h"
#include "profile.h"

/* Time service definitions */
/* Invalid date */
#define BLE_INVALID_DATE_PARAMETER  (0)

typedef struct PACKED
{
  uint16 year;
  uint8  month;
  uint8  day;
  uint8  hour;
  uint8  minute;
  uint8  second;
} ble_char_time_t;

/* Measurement interval characteristics data length */
#define BLE_MEAS_INTERVAL_UUID         (0x2a21)
#define BLE_MEAS_INTERVAL_LENGTH       (2)
#define BLE_INVALID_MEAS_INTERVAL      (0x7fffffff)

/* Temperature service definitions */
/* Update interval in millisec */
#define BLE_TEMPERATURE_SERVICE_UUID   (0x1809)
#define BLE_TEMPERATURE_MEAS_UUID      (0x2a1c)
#define BLE_TEMPERATURE_TYPE_UUID      (0x2a1d)
#define BLE_TEMPERATURE_MEAS_INTERVAL  (2*60*1000)

typedef struct PACKED
{
  uint8           flags;
  float           meas_value;
  ble_char_time_t meas_time;
  uint8           type;
} ble_char_temperature_t;

static void ble_update_temperature (int32 device_id, void *data);

static FILE *temperature_file[256];


static void ble_update_char_type (ble_attr_list_entry_t * desc_list_entry, uint8 type)
{
  if (type == BLE_CHAR_TYPE_READ)
  {
    if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_READ)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_READ;
    }
    else if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_INDICATE)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_INDICATE;
    }
    else if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_NOTIFY)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_NOTIFY;
    }
    else
    {
      desc_list_entry->declaration.type = 0;
    }    
  }
  else
  {
    if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_WRITE)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_WRITE;
    }
    else if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_WRITE_NO_RSP)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_WRITE_NO_RSP;
    }
    else if (desc_list_entry->declaration.type & BLE_CHAR_TYPE_WRITE_SIGNED)
    {
      desc_list_entry->declaration.type = BLE_CHAR_TYPE_WRITE_SIGNED;
    }
    else
    {
      desc_list_entry->declaration.type = 0;
    }
  }

  if ((desc_list_entry->declaration.type == BLE_CHAR_TYPE_INDICATE) ||
      (desc_list_entry->declaration.type == BLE_CHAR_TYPE_NOTIFY))
  {
    uint16 bitfield = (desc_list_entry->declaration.type == BLE_CHAR_TYPE_INDICATE)
                      ? BLE_CHAR_CLIENT_INDICATE : BLE_CHAR_CLIENT_NOTIFY;

    desc_list_entry = ble_find_char_desc (desc_list_entry, BLE_GATT_CHAR_CLIENT_CONFIG);
    
    if (desc_list_entry != NULL)
    {
      desc_list_entry->data_length            = sizeof (ble_char_client_config_t);
      desc_list_entry->client_config.bitfield = bitfield;
    }
  }
}

ble_attr_list_entry_t * ble_find_char_desc (ble_attr_list_entry_t *attr_list_entry,
                                            uint16 uuid)
{
  while (attr_list_entry != NULL)
  {
    if ((attr_list_entry->uuid_length == BLE_GATT_UUID_LENGTH) &&
        ((BLE_PACK_GATT_UUID (attr_list_entry->uuid)) == uuid))
    {
      break;
    }
      
    attr_list_entry = attr_list_entry->next;
  }

  return attr_list_entry;
}


static void ble_update_temperature (int32 device_id, void *data)
{
  int32 update_failed = 0;
  ble_service_list_entry_t *service_list_entry = (ble_service_list_entry_t *)data;
  ble_char_list_entry_t *update_list_entry = service_list_entry->update.char_list;

  printf ("Device ID: %02d\n", device_id);

  service_list_entry->update.expected_time += BLE_TEMPERATURE_MEAS_INTERVAL;
  service_list_entry->update.pending        = 1;

  while (update_list_entry != NULL)
  {
    ble_char_list_entry_t *update_list_entry_tmp = NULL;
    ble_attr_list_entry_t *char_value
      = (ble_attr_list_entry_t *)list_tail ((list_entry_t **)(&(update_list_entry->desc_list)));
    uint16 uuid = BLE_PACK_GATT_UUID (char_value->uuid);

    if (uuid == BLE_TEMPERATURE_MEAS_UUID)
    {
      if (char_value->data_length != 0)
      {
        ble_char_temperature_t *temperature = (ble_char_temperature_t *)(char_value->data);
      
        fprintf (temperature_file[device_id-1], "%.1f\n", temperature->meas_value);
        
        printf ("  Temperature flags: 0x%02x\n", temperature->flags);
        printf ("              value: %.1f\n", temperature->meas_value);
        printf ("               date: %02d/%02d/%04d\n", temperature->meas_time.day,
                                                         temperature->meas_time.month,
                                                         temperature->meas_time.year);
        printf ("               time: %02d:%02d:%02d\n", temperature->meas_time.hour,
                                                         temperature->meas_time.minute,
                                                         temperature->meas_time.second);
        printf ("               type: 0x%02x\n", temperature->type);
        printf ("   Timer correction: %d\n", service_list_entry->update.timer_correction);
      }
      else
      {
        update_failed = 1;

        fprintf (temperature_file[device_id-1], "NA\n");
        printf ("  Temperature not read\n");
      }
    }
    else if ((uuid == BLE_TEMPERATURE_TYPE_UUID) ||
             (uuid == BLE_MEAS_INTERVAL_UUID))
    {
      update_list_entry_tmp = update_list_entry;
    }
    else
    {
      printf ("BLE unexpected uuid 0x%04x in temperature update list\n", uuid);
    }

    if (char_value->data_length != 0)
    {
      free (char_value->data);
      char_value->data        = NULL;
      char_value->data_length = 0;
    }

    update_list_entry = update_list_entry->next;
    if (update_list_entry_tmp != NULL)
    {
      list_remove ((list_entry_t **)(&(service_list_entry->update.char_list)), (list_entry_t *)update_list_entry_tmp);
      free (update_list_entry_tmp);
      update_list_entry_tmp = NULL;
    }
  }

  if (update_failed > 0)
  {
    ble_char_list_entry_t *char_list_entry = service_list_entry->char_list;
    
    /* Add measurement interval to update list */
    while (char_list_entry != NULL)
    {
      uint8 uuid_length;
      uint16 uuid;
      ble_attr_list_entry_t *desc_list_entry
        = ble_find_char_desc (char_list_entry->desc_list, BLE_GATT_CHAR_DECL);;
      
      uuid_length = desc_list_entry->data_length - 3;
      uuid        = BLE_PACK_GATT_UUID (desc_list_entry->declaration.uuid);

      if ((uuid_length == BLE_GATT_UUID_LENGTH) && (uuid == BLE_MEAS_INTERVAL_UUID))
      {
        update_list_entry = (ble_char_list_entry_t *)malloc (sizeof (*update_list_entry));
        *update_list_entry = *char_list_entry;
        list_add ((list_entry_t **)(&(service_list_entry->update.char_list)), (list_entry_t *)update_list_entry);

        desc_list_entry = (ble_attr_list_entry_t *)list_tail ((list_entry_t **)(&(char_list_entry->desc_list)));
        desc_list_entry->data_length = BLE_MEAS_INTERVAL_LENGTH;
        desc_list_entry->data        = malloc (desc_list_entry->data_length);
        desc_list_entry->data[0]     = (BLE_TEMPERATURE_MEAS_INTERVAL/1000) & 0xff;
        desc_list_entry->data[1]     = ((BLE_TEMPERATURE_MEAS_INTERVAL/1000) >> 8) & 0xff;      
  
        service_list_entry->update.pending++;
        break;
      }      
      
      char_list_entry = char_list_entry->next;
    }
  }

  fflush (temperature_file[device_id-1]);
}

int32 ble_lookup_service (ble_service_list_entry_t *service_list_entry)
{
  int32 found = 0;
  uint8 uuid_length;
  uint16 uuid;

  service_list_entry->update.char_list        = NULL;
  service_list_entry->update.pending          = 0;
  service_list_entry->update.init             = 1;
  service_list_entry->update.expected_time    = 0;
  service_list_entry->update.timer            = 0;
  service_list_entry->update.timer_correction = 0;
  service_list_entry->update.callback         = NULL;

  uuid_length = service_list_entry->declaration.data_length;
  uuid        = BLE_PACK_GATT_UUID (service_list_entry->declaration.data);

  if ((uuid_length == BLE_GATT_UUID_LENGTH) && (uuid == BLE_TEMPERATURE_SERVICE_UUID))
  {
    ble_char_list_entry_t *char_list_entry = service_list_entry->char_list;

    printf ("BLE Found temperature service\n");
    
    /* Temperature service; add temperature measurement, temperature type 
     * and measurement interval to update list */
    while (char_list_entry != NULL)
    {
      ble_attr_list_entry_t *desc_list_entry
        = ble_find_char_desc (char_list_entry->desc_list, BLE_GATT_CHAR_DECL);;
      
      uuid_length = desc_list_entry->data_length - 3;
      uuid        = BLE_PACK_GATT_UUID (desc_list_entry->declaration.uuid);
      
      if ((uuid_length == BLE_GATT_UUID_LENGTH) &&
          ((uuid == BLE_TEMPERATURE_MEAS_UUID) ||
           (uuid == BLE_TEMPERATURE_TYPE_UUID) ||
           (uuid == BLE_MEAS_INTERVAL_UUID)))
      {
        ble_char_list_entry_t *update_list_entry = (ble_char_list_entry_t *)malloc (sizeof (*update_list_entry));
        ble_attr_list_entry_t *char_value;

        char_value = (ble_attr_list_entry_t *)malloc (sizeof (*char_value));
        char_value->handle      = desc_list_entry->declaration.handle;
        char_value->uuid_length = uuid_length;
        memcpy (char_value->uuid, desc_list_entry->declaration.uuid, uuid_length);
        char_value->data_length = 0;
        char_value->data        = NULL;
        list_add ((list_entry_t **)(&(char_list_entry->desc_list)), (list_entry_t *)char_value);

        *update_list_entry = *char_list_entry;
        list_add ((list_entry_t **)(&(service_list_entry->update.char_list)), (list_entry_t *)update_list_entry);

        if ((uuid == BLE_TEMPERATURE_MEAS_UUID) ||
            (uuid == BLE_TEMPERATURE_TYPE_UUID))
        {
          ble_update_char_type (desc_list_entry, BLE_CHAR_TYPE_READ);         
        }
        else
        {
          ble_update_char_type (desc_list_entry, BLE_CHAR_TYPE_WRITE);          

          char_value->data_length  = BLE_MEAS_INTERVAL_LENGTH;
          char_value->data         = malloc (desc_list_entry->data_length);
          char_value->data[0]      = (BLE_TEMPERATURE_MEAS_INTERVAL/1000) & 0xff;
          char_value->data[1]      = ((BLE_TEMPERATURE_MEAS_INTERVAL/1000) >> 8) & 0xff;
        }

        found++;
      }

      char_list_entry = char_list_entry->next;
    }

    if (found > 0)
    {
      service_list_entry->update.pending  = 3;
      service_list_entry->update.callback = ble_update_temperature;
    }
  }

  return found;  
}

uint32 ble_identify_device (uint8 *address, ble_service_list_entry_t *service_list_entry)
{
  int32 id = 0;
  int8 *file_name;
  
  if (service_list_entry != NULL)
  {
    id = address[0];

    if ((asprintf (&file_name, "temperature.%03d", id)) > 0)
    {
      temperature_file[id-1] = fopen (file_name, "w");
      printf ("Storing temperature in file %s\n", file_name);
      free (file_name);
    }
  }

  return id;
}
