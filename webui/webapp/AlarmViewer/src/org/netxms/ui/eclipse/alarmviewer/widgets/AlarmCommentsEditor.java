/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.alarmviewer.widgets;

import java.text.DateFormat;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.api.client.users.AbstractUserObject;
import org.netxms.client.NXCSession;
import org.netxms.client.events.AlarmNote;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.ImageCache;

/**
 * Editor for alarm comments
 */
public class AlarmCommentsEditor extends Composite
{
	private static final long serialVersionUID = 1L;

	private Text text;
	
	/**
	 * @param parent
	 * @param style
	 * @param note
	 */
	public AlarmCommentsEditor(Composite parent, FormToolkit toolkit, ImageCache imageCache, AlarmNote note)
	{
		super(parent, SWT.BORDER);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		setLayout(layout);
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final AbstractUserObject userObject = session.findUserDBObjectById(note.getUserId());
		
		final CLabel user = new CLabel(this, SWT.NONE);
		toolkit.adapt(user);
		user.setImage(imageCache.add(Activator.getImageDescriptor("icons/user.png")));
		user.setText((userObject != null) ? userObject.getName() : "<unknown>");
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = false;
		user.setLayoutData(gd);
		
		final Label time = toolkit.createLabel(this, DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.LONG).format(note.getLastChangeTime()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		time.setLayoutData(gd);
		
		final ImageHyperlink linkEdit = toolkit.createImageHyperlink(this, SWT.NONE);
		linkEdit.setText("Edit");
		linkEdit.setImage(SharedIcons.IMG_EDIT);
		linkEdit.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
			}
		});
		gd = new GridData();
		gd.horizontalAlignment = SWT.RIGHT;
		linkEdit.setLayoutData(gd);
		
		text = new Text(this, SWT.MULTI | SWT.READ_ONLY);
		text.setText(note.getText());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 3;
		text.setLayoutData(gd);
	}
}
