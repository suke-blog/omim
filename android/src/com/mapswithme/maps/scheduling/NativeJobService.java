package com.mapswithme.maps.scheduling;

import android.annotation.TargetApi;
import android.app.job.JobParameters;
import android.app.job.JobService;
import android.os.Build;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

@TargetApi(Build.VERSION_CODES.LOLLIPOP)
public class NativeJobService extends JobService
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = NativeJobService.class.getSimpleName();

  @Override
  public boolean onStartJob(JobParameters params)
  {
    LOGGER.d(TAG, "onStartJob");
    JobServiceDelegate delegate = new JobServiceDelegate(getApplication());
    delegate.onStartJob();
    return true;
  }

  @Override
  public boolean onStopJob(JobParameters params)
  {
    return false;
  }
}
