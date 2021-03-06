<?php

/**
 *  call back funciton for pulling data from google anaylics
 *  @param metrics_in : and array or string of the metrics to pull
 *  @param start_date : time as an int
 *  @param end_date : time as an int
 *  @return : and array of external_statistics_count db obj
 *
 */
function ga_stats_ga_data($metrics, $start_date = 0, $end_date = 0, $filter=false) {
  $url_dim = 'pagePath';
  if (!is_array($metrics)) {
   $metrics = array($metrics);
  }
  $request['dimensions'] = array($url_dim);
  $request['metrics'] = $metrics;
  
  if($start_date)
    $request['start_date'] = date('Y-m-d', $start_date);
  else
    $request['start_date'] = null;
    
  if($end_date)
    $request['end_date'] = date('Y-m-d', $end_date);
  else
    $request['end_date'] = null;
  
  $request['sort_metric'] = "-".$metrics[0];
  $request['max_results'] = variable_get('ga_stats_max_results', "100");
  
  if($filter){
    $request['filter'] = $filter;
  }
  
  $data_raw = ga_stats_query_data($request);
  $data_array = ga_stats_ga_data_array($data_raw);
  foreach($data_array as $k=>$d) {
    $data_array[$k]['url'] = $d[$url_dim];
  }
  
  return $data_array;
}

/*
 * Helper function to list metrics with plugin names and google analytics names
 */
function ga_stats_ga_metrics($all=false) {
  
  $metrics = array(
    'pageviews'=>'Page Views',
    'uniquePageviews'=>'Unique Page Views',
    'avgTimeOnPage'=>'Average Time on Page',
    'entrances'=>'Entrance Page'
  );
  
  if(!$all){
    $enabled = variable_get('ga_stats_enabled_metrics', array('pageviews'));
    foreach($metrics as $k=>$v){
      if(!$enabled[$k]){
        unset($metrics[$k]);
      }
    }
  }
  
  return $metrics;
}
/**
 *  @param $just_keys : whether we should return all the data or just the keys->values array
 *  TODO: add forever
 */
function ga_stats_ga_timeframes($just_keys=false, $all=false){
  if($just_keys){
    $timeframes = array(
      'hour' => 'in the past hour',
      'today' => 'in the past 24 hours',
      'week' => 'in the past 7 days',
      'month' => 'in the past 31 days',
      'forever' => 'for all time'
    );
  } else {
    $timeframes = array(
      'hour'=>array('secsToSub'=>(60*60), 'filter'=>'hour=='+date('G'), 'title'=>'in the past hour'),
      'today'=>array('secsToSub'=>(60*60*24), 'filter'=>'day=='+date('j'), 'title'=>'in the past 24 hours'),
      'week'=>array('secsToSub'=>(60*60*24*7), 'title'=>'in the past 7 days'),
      'month'=>array('secsToSub'=>(60*60*24*31), 'title'=>'in the past 31 days'),
      'forever'=>array('secsToSub'=>time() - strtotime('2005-01-01'), 'title'=>'for all time')
    );
  }
  
  if(!$all){
    $enabled = variable_get('ga_stats_enabled_timeframes', array('today', 'month'));
    foreach($timeframes as $k=>$v){
      if(!$enabled[$k]){
        unset($timeframes[$k]);
      }
    }
  }
  
  return $timeframes;
}

function ga_stats_ga_data_array($data_in) {
  $data_all = array();
  $data = array();

  foreach ($data_in as $d) {
    $metrics = $d->getMetrics();
    $dimensions = $d->getDimensions();
    $data_all[] = array_merge($metrics, $dimensions);
  }
  return $data_all;
}

function ga_stats_ga_get_accounts(){
  require_once 'gapi.class.php';
  
  $user = variable_get('ga_stats_email', '');
  $password = variable_get('ga_stats_password', '');
    
  if($user && $password){
    try{
      $ga = new gapi($user, $password);
      $accounts = $ga->requestAccountData(1, 5000); 
      return $accounts;
    } catch (Exception $e) {
      drupal_set_message('Invalid Google Analytics login.', 'error');
      watchdog('ga_stats', 'Invalid Google Analytics login.');
      return;
    }    
  } else {
    drupal_set_message(t('Google Analytics Email and password not set - Could not retrieve Account Information.'), 'error');
    watchdog('ga_stats', t('Google Analytics email and password not set.'));
  }
}

/**
 *  @param $metric - one of the metrics values - a string value - i.e. 'pageviews'
 *  $report_id, $dimensions, $metrics, $sort_metric=null, $filter=null, $start_date=null, $end_date=null, $start_index=1, $max_results=30
 */

function ga_stats_query_data($request){
  //this is really a waste I think because this shit is preprocessed
  require_once 'gapi.class.php';
  
  $user = variable_get('ga_stats_email', '');
  $password = variable_get('ga_stats_password', '');
  $aid = variable_get('ga_stats_profile', '');
  
  if($user && $password){
    try{
      $ga = new gapi($user, $password);
      $data = $ga->requestReportData($aid, $request['dimensions'], $request['metrics'], $request['sort_metric'], null, $request['start_date'], $request['end_date'], 1, $request['max_results']);
    } catch (Exception $e) {
      drupal_set_message('Invalid Google Analytics login.', 'error');
      watchdog('ga_stats', 'Invalid Google Analytics login.');
      return array();
    }    
    return $data;    
  } else {
    drupal_set_message('Google Analytics Email and password not set.', 'error');
    watchdog('ga_stats', 'Google Analytics email and password not set.');
  }
}