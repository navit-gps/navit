package org.navitproject.navit;


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.RelativeLayout.LayoutParams;


public class NavitAddressSearchActivity extends Activity
{
	private EditText			address_string;
	private CheckBox			pm_checkbox;

	public RelativeLayout	NavitAddressSearchActivity_layout;

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);

		getWindow().setFlags(WindowManager.LayoutParams.FLAG_BLUR_BEHIND,
				WindowManager.LayoutParams.FLAG_BLUR_BEHIND);
		LinearLayout panel = new LinearLayout(this);
		panel.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT));
		panel.setOrientation(LinearLayout.VERTICAL);

		// address: label and text field
		TextView addr_view = new TextView(this);
		addr_view.setText("Type Address");
		addr_view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20f);
		addr_view.setLayoutParams(new LayoutParams(LayoutParams.WRAP_CONTENT,
				LayoutParams.WRAP_CONTENT));
		addr_view.setPadding(4, 4, 4, 4);

		// partial match checkbox
		pm_checkbox = new CheckBox(this);
		pm_checkbox.setText("partial match");
		pm_checkbox.setChecked(false);
		pm_checkbox.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.WRAP_CONTENT));
		pm_checkbox.setGravity(Gravity.CENTER);


		// search button
		final Button btnSearch = new Button(this);
		btnSearch.setText("Search");
		btnSearch.setLayoutParams(new LayoutParams(LayoutParams.FILL_PARENT,
				LayoutParams.WRAP_CONTENT));
		btnSearch.setGravity(Gravity.CENTER);
		btnSearch.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				executeDone();
			}
		});


		// title
		try
		{
			String s = getIntent().getExtras().getString("title");
			if (s.length() > 0)
			{
				this.setTitle(s);
			}
		}
		catch (Exception e)
		{
		}
		
		// partial match
		try
		{
			String s = getIntent().getExtras().getString("partial_match");
			if (s.length() > 0)
			{
				if (s.equals("1"))
				{
					pm_checkbox.setChecked(true);
				}
				else
				{
					pm_checkbox.setChecked(false);
				}
			}
		}
		catch (Exception e)
		{
		}

		// address string
		try
		{
			address_string = new EditText(this);
			address_string.setText(getIntent().getExtras().getString("address_string"));
		}
		catch (Exception e)
		{
		}


		// actually adding the views (that have layout set on them) to the panel
		panel.addView(addr_view);
		panel.addView(address_string);
		panel.addView(pm_checkbox);
		panel.addView(btnSearch);

		setContentView(panel);

	}

	private void executeDone()
	{
		Intent resultIntent = new Intent();
		resultIntent.putExtra("address_string", NavitAddressSearchActivity.this.address_string
				.getText().toString());
		if (NavitAddressSearchActivity.this.pm_checkbox.isChecked())
		{
			resultIntent.putExtra("partial_match", "1");
		}
		else
		{
			resultIntent.putExtra("partial_match", "0");
		}
		setResult(Activity.RESULT_OK, resultIntent);
		finish();
	}
}
