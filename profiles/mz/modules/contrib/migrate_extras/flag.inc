<?php

/**
 * @file
 * Flag module integration
 */

/**
 * Destination class implementing when you want just an insert into flag_content table.
 */
class MigrateDestinationFlagSimple extends MigrateDestination {


  protected $table_name;
  protected $flagging_id;
  protected $entity_type;
  protected $entity_id;


  public function __construct($fid) {
    parent::__construct();

    if (FLAG_API_VERSION == 3) {
      $this->table_name = 'flagging';
      $this->entity_id = 'entity_id';
      $this->entity_type = 'entity_type';
      $this->flagging_id = 'flagging_id';
    }
    else {
      $this->table_name = 'flag_content';
      $this->entity_id = 'content_type';
      $this->entity_type = 'content_id';
      $this->flagging_id = 'fcid';
    }

    $this->fid = $fid;
  }

  public function __toString() {
    $flag = flag_get_flag(NULL, $this->fid);
    return t('flag (!flag)', array('!flag' => $flag->name));
  }

  static public function getKeySchema() {

    if (FLAG_API_VERSION == 3) {

      $flagging_id = 'flagging_id';
    }
    else {
      $flagging_id = 'fcid';
    }


    return array(
      $flagging_id => array(
        'type' => 'int',
        'not null' => TRUE,
      ),
    );
  }

  /**
   * Delete a membership.
   *
   * @param $id
   *  IDs to be deleted.
   */
  public function bulkRollback(array $ids) {
    migrate_instrument_start(__METHOD__);

    db_delete($this->table_name)
      ->condition($this->flagging_id, $ids, 'IN')
      ->execute();

    migrate_instrument_stop(__METHOD__);
  }

  /**
   * Import a single flag_content.
   *
   * @param $entity
   *  Object object to build. Prefilled with any fields mapped in the Migration.
   * @param $row
   *  Raw source data object - passed through to prepare/complete handlers.
   * @return array
   *  Array of key fields of the object that was saved if
   *  successful. FALSE on failure.
   */
  public function import(stdClass $entity, stdClass $row) {


    // Updating previously-migrated content?
    if (isset($row->migrate_map_destid1)) {
      if (isset($entity->{$this->flagging_id})) {
        if ($entity->{$this->flagging_id} != $row->migrate_map_destid1) {
          throw new MigrateException(t("Incoming id !id and map destination id !destid don't match.",
            array('!id' => $entity->{$this->flagging_id}, '!destid' => $row->migrate_map_destid1)));
        }
      }
      else {
        $entity->{$this->flagging_id} = $row->migrate_map_destid1;
      }
    }
    // Otherwise, clear out the fcid, just to be safe.
    else {
      unset($entity->{$this->flagging_id});
    }


    if (isset($entity->timestamp)) {
      $entity->timestamp = Migration::timestamp($entity->timestamp);
    }

    $entity->fid = $this->fid;

    if (!empty($entity->{$this->flagging_id})) {
      $return = drupal_write_record($this->table_name, $entity, array($this->flagging_id));
    }
    else {
      $return = drupal_write_record($this->table_name, $entity);
    }
    if ($return) {
      // Increment the number of updated or inserted records by checking the
      // result of drupal_write_record.
      ($return == SAVED_NEW) ? $this->numCreated++ : $this->numUpdated++;

      // Update the flag_counts table.
      $count = db_select($this->table_name)
        ->fields($this->table_name)
        ->condition('fid', $this->fid)
        ->condition($this->entity_type, $entity->{$this->entity_type})
        ->condition($this->entity_id, $entity->{$this->entity_id})
        ->countQuery()
        ->execute()
        ->fetchField();
      db_merge('flag_counts')
        ->key(array(
          'fid' => $this->fid,
          $this->entity_id => $entity->{$this->entity_id},
        ))
        ->fields(array(
          $this->entity_type => $entity->{$this->entity_type},
          'count' => $count,
          'last_updated' => REQUEST_TIME,
        ))
        ->execute();
      return array($entity->{$this->flagging_id});
    }
  }

  public function fields() {
    return array(
      $this->flagging_id => 'Flag content ID',
      'fid' => 'Flag ID',
      $this->entity_type => '',
      $this->entity_id => '',
      'uid' => 'User ID',
      'timestamp' => '',
    );
  }
}

/**
 * Field handler - this will expose flags as fields on the object they're
 * attached to, and set the flag to the value mapped in addFieldMapping().
 */
abstract class MigrateExtrasFlagHandler extends MigrateDestinationHandler {
  /**
   * Make the flags assigned to this object visible.
   */
  public function fields($entity_type, $bundle) {
    $fields = array();
    if (module_exists('flag')) {
      $flags = flag_get_flags($entity_type, $bundle);
      foreach ($flags as $flag_name => $flag) {
        $fields[$flag_name] = $flag->title;
      }
    }
    return $fields;
  }
}

/**
 * Because we can't identify what kind of entity is passed to complete, we
 * implement a separate handler for each type.
 */
class MigrateExtrasNodeFlagHandler extends MigrateExtrasFlagHandler {
  public function __construct() {
    $this->registerTypes(array('node'));
  }

  public function complete($node, stdClass $row) {
    if (!module_exists('flag')) {
      return;
    }

    $flags = flag_get_flags('node', $node->type);
    $fields = array();
    foreach ($flags as $flag_name => $flag) {
      if (!empty($node->$flag_name)) {
        flag('flag', $flag_name, $node->nid);
      }
    }
  }
}

class MigrateExtrasUserFlagHandler extends MigrateExtrasFlagHandler {
  public function __construct() {
    $this->registerTypes(array('user'));
  }

  public function complete($user, stdClass $row) {
    if (!module_exists('flag')) {
      return;
    }

    $flags = flag_get_flags('user', 'user');
    $fields = array();
    foreach ($flags as $flag_name => $flag) {
      if (!empty($user->$flag_name)) {
        flag('flag', $flag_name, $user->uid);
      }
    }
  }
}

class MigrateExtrasTaxonomyFlagHandler extends MigrateExtrasFlagHandler {
  public function __construct() {
    $this->registerTypes(array('taxonomy_term'));
  }

  public function complete($term, stdClass $row) {
    if (!module_exists('flag')) return;

    $flags = flag_get_flags('taxonomy_term', $term->vocabulary_machine_name);
    $fields = array();
    
    foreach ($flags as $flag_name => $flag) {
      if (!empty($term->$flag_name)) {
        flag('flag', $flag_name, $term->tid);
      }
    }
    
  }
}